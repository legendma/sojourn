#include "pch.hpp"
#include "network_main.hpp"
#include "network_matchmaking.hpp"

#include "common/engine/engine_utilities.hpp"

Engine::Networking::Networking() :
    m_crypto_map_next_id( 1 )
{
    Initialize();
}

Engine::Networking::~Networking()
{
    WSACleanup();
}

void Engine::Networking::Initialize()
{
    /* start WinSock */
    auto result = WSAStartup( MAKEWORD( 2, 2 ), &m_wsa_data );
    if( result != NO_ERROR )
    {
        Engine::ReportError( L"Networking::Initialize could not initialize Winsock DLL." );
        throw std::runtime_error( "WSAStartup" );
    }

}

Engine::NetworkPacketPtr Engine::Networking::ReadPacket( uint64_t protocol_id, NetworkKey &read_key, NetworkPacketTypesAllowed &allowed, uint64_t now_time, InputBitStreamPtr &read )
{
    auto marker = read->SaveCurrentLocation();
    Engine::NetworkPacketPrefix prefix;
    read->Read( prefix.b );
    read->SeekToLocation( marker );
    if( prefix.packet_type == Engine::PACKET_CONNECT_REQUEST )
    {
        /* handle new connection request */
        Engine::NetworkConnectionRequestHeader request;
        ::ZeroMemory( &request, sizeof(request) );

        if( !allowed.IsAllowed( Engine::PACKET_CONNECT_REQUEST ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request.  Not allowed." ).c_str() );
            return nullptr;
        }
        else if( read->GetSize() != sizeof( Engine::NetworkConnectionRequestHeader ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request.  Bad packet size.  Expected %d, got %d." ).c_str(), sizeof( Engine::NetworkConnectionRequestHeader ), read->GetSize() );
        }

        read->Advance( sizeof( prefix ) );

        /* test if protocol version matches */
        read->ReadBytes( &request.version, request.version.size() );
        if( std::string( request.version.data() ) != std::string( NETWORK_PROTOCOL_VERSION ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request.  Protocol version was incorrect." ).c_str() );
            return nullptr;
        }

        /* test if protocol ID matches */
        read->Read( request.protocol_id );
        if( request.protocol_id != protocol_id )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request.  Invalid Protocol ID." ).c_str() );
            return nullptr;
        }

        /* test if the connection token has expired */
        read->Read( request.token_expire_time );
        if( now_time > request.token_expire_time )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request.  Connection token expired." ).c_str() );
            return nullptr;
        }

        read->Read( request.token_sequence );

        read->ReadBytes( &request.raw_token, sizeof(request.raw_token ) );
        if( !NetworkConnectionToken::Decrypt( request.raw_token, request.token_sequence, protocol_id, now_time, Engine::SOJOURN_PRIVILEGED_KEY ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request.  Could not decrypt." ).c_str() );
            return nullptr;
        }

        NetworkConnectionTokenPtr plain_token( new NetworkConnectionToken() );
        if( !plain_token->Read( request.raw_token ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request.  Could not read connection token." ).c_str() );
            return nullptr;
        }
        
        auto packet = Engine::NetworkPacketFactory::CreateIncomingConnectionRequest( request, plain_token );
        return packet;
    }

    return nullptr;
}

void Engine::Networking::SendPacket( Engine::NetworkSocketUDPPtr &socket, NetworkAddressPtr &to, NetworkPacketPtr &packet, uint64_t protocol_id, NetworkKey &key, uint64_t sequence_num )
{
    auto buffer = packet->WritePacket( sequence_num, protocol_id, key );
    socket->SendTo( buffer->GetBuffer(), buffer->GetSize(), to );
}

uint32_t Engine::Networking::AddCryptoMap( NetworkAddressPtr &client_address, NetworkKey &send_key, NetworkKey &receive_key, uint64_t now_time, uint64_t expire_time, int timeout_secs )
{
    NetworkCryptoMapPtr free_record;
    uint32_t out_id;
    for( auto mapping : m_crypto_map )
    {
        auto crypto = mapping.second;
        auto expired = crypto->IsExpired( now_time );
        if( expired
         && !free_record )
        {
            free_record = crypto;
            out_id = mapping.first;
        }

        if( !expired
          && crypto->address->Matches( *client_address ) )
        {
            /* found an existing mapping from this client, so just update it */
            crypto->expire_time = expire_time;
            crypto->timeout_seconds = timeout_secs;
            crypto->last_seen = now_time;
            crypto->send_key = send_key;
            crypto->receive_key = receive_key;            

            return mapping.first;
        }
    }

    /* use expired record if found, rather than always creating new */
    NetworkCryptoMapPtr new_record = free_record;
    if( !new_record )
    {
        new_record = NetworkCryptoMapPtr( new NetworkCryptoMap() );
        out_id = m_crypto_map_next_id++;
        m_crypto_map.insert( std::make_pair( out_id, new_record ) );
    }

    new_record->address = client_address;
    new_record->expire_time = expire_time;
    new_record->timeout_seconds = timeout_secs;
    new_record->last_seen = now_time;
    new_record->send_key = send_key;
    new_record->receive_key = receive_key;

    return out_id;
}

Engine::NetworkCryptoMapPtr Engine::Networking::FindCryptoMapByAddress( NetworkAddressPtr &search_address, uint64_t time )
{
    for( auto mapping : m_crypto_map )
    {
        auto crypto = mapping.second;
        if( !crypto->IsExpired( time )
          && crypto->address->Matches( *search_address ) )
        {
            crypto->last_seen = time;
            return crypto;
        }
    }

    return nullptr;
}

Engine::NetworkCryptoMapPtr Engine::Networking::FindCryptoMapByID( uint32_t search_id, NetworkAddressPtr &expected_address, uint64_t time )
{
    assert( expected_address != nullptr );
    auto found = m_crypto_map.find( search_id );
    if( found == m_crypto_map.end() )
    {
        return nullptr;
    }

    auto mapping = found->second;
    if( !mapping->address->Matches( *expected_address )
      || mapping->IsExpired( time ) )
    {
        return nullptr; 
    }

    /* update last seen */
    mapping->last_seen = time;

    return mapping;
}

bool Engine::Networking::Encrypt( void *data_to_encrypt, size_t data_length, byte *salt, size_t salt_length, NetworkNonce &nonce, const NetworkKey &key )
{
    unsigned long long encrypted_length;
    auto data = reinterpret_cast<unsigned char*>( data_to_encrypt );
    auto result = crypto_aead_chacha20poly1305_ietf_encrypt( data, &encrypted_length,
                                                             data, static_cast<unsigned long long>(data_length) - sizeof(NetworkAuthentication),
                                                             salt, salt_length,
                                                             nullptr, nonce.data(), key.data() );

    assert( encrypted_length == data_length );
    return result == 0;
}

bool Engine::Networking::Decrypt( void *data_to_decrypt, size_t data_length, byte *salt, size_t salt_length, NetworkNonce &nonce, const NetworkKey &key )
{
    unsigned long long decrypted_length;
    auto data = reinterpret_cast<unsigned char*>(data_to_decrypt);
    auto result = crypto_aead_chacha20poly1305_ietf_decrypt( data, &decrypted_length,
                                                             nullptr, data, static_cast<unsigned long long>(data_length) - sizeof( NetworkAuthentication ),
                                                             salt, salt_length,
                                                             nonce.data(), key.data() );

    assert( decrypted_length == data_length );
    return result == 0;
}

void Engine::Networking::GenerateEncryptionKey( NetworkKey &key )
{
    crypto_aead_chacha20poly1305_ietf_keygen( key.data() );
}

void Engine::Networking::GenerateRandom( byte *out, size_t size )
{
    randombytes_buf( out, size );
}

Engine::NetworkingPtr Engine::NetworkingFactory::StartNetworking()
{
    try
    {
        return ( NetworkingPtr( new Networking() ) );
    }
    catch( std::runtime_error() ) {}
      
    return( nullptr );
}

Engine::BitStreamBase::BitStreamBase( bool is_owned ) :
    m_buffer( nullptr ),
    m_bit_head( 0 ),
    m_bit_capacity( 0 ),
    m_owned( is_owned )
{
}

void Engine::BitStreamBase::ReallocateBuffer( const size_t size )
{
    assert( m_owned );
    if( !m_buffer )
    {
        BindBuffer( static_cast<byte*>( std::malloc( size ) ), size );
    }
    else
    {
        BindBuffer( static_cast<byte*>( std::realloc( m_buffer, size ) ), size );
    }
}

void Engine::BitStreamBase::BindBuffer( byte* buffer, const size_t size )
{
    m_buffer = buffer;
    m_bit_capacity = 8 * size;
}

int Engine::BitStreamBase::BitsRequired( uint64_t value )
{
    int required_bits = 0;
    while( value )
    {
        value >>= 1;
        required_bits++;
    }

    return required_bits;
}

int Engine::BitStreamBase::BytesRequired( uint64_t value )
{
    int required_bits = BitsRequired( value );
    return ( required_bits + 7 ) / 8;
}

Engine::InputBitStream::InputBitStream( byte *input, const size_t size, bool owned ) :
    Engine::BitStreamBase( owned )
{
    if( !owned )
    {
        BindBuffer( input, size );
        return;
    }

    ReallocateBuffer( size );
    std::memcpy( m_buffer, input, size );
}

void Engine::InputBitStream::ReadBits( byte &out, uint32_t bit_cnt )
{
    uint32_t byte_offset = m_bit_head / 8;
    uint32_t bit_offset  = m_bit_head % 8;

    out = m_buffer[byte_offset] >> bit_offset;

    auto free_bits_this_byte = 8 - bit_offset;
    if( free_bits_this_byte < bit_cnt )
    {
        /* go to the next byte */
        out |= m_buffer[byte_offset + 1] << free_bits_this_byte;
    }

    out &= ~( 0xff << bit_cnt);
    m_bit_head += bit_cnt;
}

void Engine::InputBitStream::Read( NetworkKey & out )
{
    for( size_t i = 0; i < out.size(); i++ )
    {
        Read( out[i] );
    }
}

void Engine::InputBitStream::Read( sockaddr &out )
{
    Read( out.sa_family );
    ReadBytes( &out.sa_data, sizeof( out.sa_data ) );
}

void Engine::InputBitStream::ReadBits( void *out, uint32_t bit_cnt )
{
    auto destination = reinterpret_cast<byte*>(out);

    /* read the whole bytes */
    while( bit_cnt > 8 )
    {
        ReadBits( *destination, 8 );
        ++destination;
        bit_cnt -= 8;
    }

    /* read any left over bits */
    if( bit_cnt > 0 )
    {
        ReadBits( *destination, bit_cnt );
    }
}

Engine::OutputBitStream::OutputBitStream( byte *input, const size_t size, bool owned ) :
    BitStreamBase( owned )
{
    if( !owned )
    {
        assert( input );
        BindBuffer( input, size );
        return;
    }

    ReallocateBuffer( size );
    if( input )
    {
        std::memcpy( m_buffer, input, size );
    }
}

void Engine::OutputBitStream::WriteBits( byte &out, uint32_t bit_cnt )
{
	auto next_bit_head = m_bit_head + bit_cnt;
	
	if( next_bit_head > m_bit_capacity )
	{
        ReallocateBuffer( std::max( m_bit_capacity * 2, next_bit_head ) );
	}
	
    uint32_t byte_offset = m_bit_head / 8;
    uint32_t bit_offset  = m_bit_head % 8;
	
	auto mask = ~( 0xff << bit_offset );
    m_buffer[byte_offset] = (m_buffer[byte_offset] & mask) | (out << bit_offset);
	
    auto free_bits_this_byte = 8 - bit_offset;
	if( free_bits_this_byte < bit_cnt )
	{
        /* go to the next byte */
		m_buffer[byte_offset + 1 ] = out >> free_bits_this_byte;
	}
	
    m_bit_head = next_bit_head;
}

void Engine::OutputBitStream::Write( NetworkKey &out )
{
    for( size_t i = 0; i < out.size(); i++ )
    {
        Write( out[i] );
    }
}

void Engine::OutputBitStream::Write( NetworkAuthentication &out )
{
    for( size_t i = 0; i < out.size(); i++ )
    {
        Write( out[i] );
    }
}

void Engine::OutputBitStream::Write( sockaddr &out )
{
    Write( out.sa_family );
    WriteBytes( out.sa_data, sizeof( out.sa_data ) );
}

size_t Engine::OutputBitStream::Collapse()
{
    assert( m_owned );
    size_t size = GetSize();
    ReallocateBuffer( size );
    return( size );
}

void Engine::OutputBitStream::WriteBits( void *out, uint32_t bit_cnt )
{
    auto source = reinterpret_cast<byte*>(out);

    /* write the whole bytes */
    while( bit_cnt > 8 )
    {
        WriteBits( *source, 8 );
        ++source;
        bit_cnt -= 8;
    }

    /* write any left over bits */
    if( bit_cnt > 0 )
    {
        WriteBits( *source, bit_cnt );
    }
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateIncomingConnectionRequest( NetworkConnectionRequestHeader &header, NetworkConnectionTokenPtr token )
{
    auto packet = std::shared_ptr<NetworkConnectionRequestPacket>( new NetworkConnectionRequestPacket() );
    packet->header = header;
    packet->token = token;

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateOutgoingConnectionDenied()
{
    auto packet = std::shared_ptr<NetworkConnectionDeniedPacket>( new NetworkConnectionDeniedPacket() );
    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateOutgoingConnectionChallenge( NetworkConnectionChallengeHeader &header, NetworkChallengeTokenRaw &token )
{
    auto packet = std::shared_ptr<NetworkConnectionChallengePacket>( new NetworkConnectionChallengePacket() );
    packet->header = header;
    packet->token = token;

    return packet;
}

Engine::OutputBitStreamPtr Engine::NetworkPacket::WritePacket( uint64_t sequence_num, uint64_t protocol_id, NetworkKey &key )
{
    auto out = BitStreamFactory::CreateOutputBitStream();
   
    /* handle connection requests without encryption */
    if( packet_type == PACKET_CONNECT_REQUEST )
    {
        Write( out );
        return out;
    }

    /* encrypted packet */
    auto sequence_byte_cnt = BitStreamBase::BytesRequired( sequence_num );

    /* write the unencrypted section */
    NetworkPacketPrefix prefix;
    prefix.packet_type = packet_type;
    prefix.sequence_byte_cnt = sequence_byte_cnt;
    out->Write( prefix.b );
    out->Write( sequence_num, sequence_byte_cnt * 8 );

    /* write the data to be encrypted */
    auto encrypted = BitStreamFactory::CreateOutputBitStream();
    Write( encrypted );

    /* create the nonce */
    NetworkNonce nonce;
    auto nonce_alias = BitStreamFactory::CreateOutputBitStream( nonce.data(), nonce.size(), false );
    nonce_alias->Write( 0, 32 );
    nonce_alias->Write( sequence_num );

    /* create the salt */
    struct
    {
        byte version[NETWORK_PROTOCOL_VERSION_LEN];
        uint64_t protocol_id;
        NetworkPacketPrefix prefix;
    } salt;

    auto salt_alias = BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>( &salt ), sizeof(salt), false );
    salt_alias->WriteBytes( (void*)NETWORK_PROTOCOL_VERSION, NETWORK_PROTOCOL_VERSION_LEN );
    salt_alias->Write( protocol_id );
    salt_alias->Write( prefix.b );

    /* pad for authentication */
    auto message_length = encrypted->GetCurrentByteCount();
    NetworkAuthentication authentication;
    encrypted->Write( authentication );

    /* encrypt and append to the output packet buffer */
    if( !Networking::Encrypt( encrypted->GetBuffer(), message_length, salt_alias->GetBuffer(), salt_alias->GetCurrentByteCount(), nonce, key ) )
    {
        return nullptr;
    }

    out->WriteBytes( encrypted->GetBuffer(), encrypted->GetSize() );
    
    return out;
}

void Engine::NetworkConnectionRequestPacket::Write( OutputBitStreamPtr &out )
{
    out->WriteBytes( header.version.data(), header.version.size() );
    out->Write( header.protocol_id );
    out->Write( header.token_expire_time );
    out->Write( header.token_sequence );

    NetworkConnectionTokenRaw raw_token;
    token->Write( raw_token );
    out->WriteBytes( reinterpret_cast<byte*>(&raw_token), sizeof( NetworkConnectionTokenRaw ) );
}

bool Engine::NetworkConnectionToken::Read( NetworkConnectionTokenRaw &raw )
{
    auto in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw), raw.size(), false );
    assert( server_address_cnt > 0 );
    assert( server_address_cnt <= (int)server_addresses.size() );

    in->Read( client_id );
    in->Read( timeout_seconds );
    in->Read( server_address_cnt );

    if( server_address_cnt <= 0
     || server_address_cnt > (int)server_addresses.size() )
    {
        return false;
    }

    for( auto i = 0; i < server_address_cnt; i++ )
    {
        in->Read( server_addresses[i] );
    }

    in->Read( client_to_server_key );
    in->Read( server_to_client_key );

    in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw) + raw.size() - sizeof(authentication), sizeof( authentication ), false );
    in->ReadBytes( authentication.data(), authentication.size() );

    return true;
}

void Engine::NetworkConnectionToken::Write( NetworkConnectionTokenRaw &raw )
{
    ::ZeroMemory( &raw, sizeof(raw) );
    auto out = Engine::BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>( &raw ), raw.size(), false );
    assert( server_address_cnt > 0 );
    assert( server_address_cnt <= (int)server_addresses.size() );

    out->Write( client_id );
    out->Write( timeout_seconds );
    out->Write( server_address_cnt );
    
    for( auto i = 0; i < server_address_cnt; i++ )
    {
        out->Write( server_addresses[ i ] );
    }

    out->Write( client_to_server_key );
    out->Write( server_to_client_key );
}

bool Engine::NetworkConnectionToken::Encrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, uint64_t &expire_time, const NetworkKey &key )
{
    /* create the nonce */
    NetworkNonce nonce;
    auto nonce_alias = BitStreamFactory::CreateOutputBitStream( nonce.data(), nonce.size(), false );
    nonce_alias->Write( 0, 32 );
    nonce_alias->Write( sequence_num );

    /* create the salt */
    struct
    {
        byte version[NETWORK_PROTOCOL_VERSION_LEN];
        uint64_t protocol_id;
        uint64_t expire_time;
    } salt;

    auto salt_alias = BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>(&salt), sizeof( salt ), false );
    salt_alias->WriteBytes( (void*)NETWORK_PROTOCOL_VERSION, NETWORK_PROTOCOL_VERSION_LEN );
    salt_alias->Write( protocol_id );
    salt_alias->Write( expire_time );

    return Networking::Encrypt( &raw, raw.size(), salt_alias->GetBuffer(), salt_alias->GetCurrentByteCount(), nonce, key );
}

bool Engine::NetworkConnectionToken::Decrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, uint64_t &expire_time, const NetworkKey &key )
{
    /* create the nonce */
    NetworkNonce nonce;
    auto nonce_alias = BitStreamFactory::CreateOutputBitStream( nonce.data(), nonce.size(), false );
    nonce_alias->Write( 0, 32 );
    nonce_alias->Write( sequence_num );

    /* create the salt */
    struct
    {
        byte version[NETWORK_PROTOCOL_VERSION_LEN];
        uint64_t protocol_id;
        uint64_t expire_time;
    } salt;

    auto salt_alias = BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>(&salt), sizeof( salt ), false );
    salt_alias->WriteBytes( (void*)NETWORK_PROTOCOL_VERSION, NETWORK_PROTOCOL_VERSION_LEN );
    salt_alias->Write( protocol_id );
    salt_alias->Write( expire_time );

    return Networking::Decrypt( &raw, raw.size(), salt_alias->GetBuffer(), salt_alias->GetCurrentByteCount(), nonce, key );
}

void Engine::NetworkConnectionChallengePacket::Write( OutputBitStreamPtr &out )
{
    out->Write( header.token_sequence );
    out->WriteBytes( token.data(), token.size() );
}

void Engine::NetworkChallengeToken::Write( NetworkChallengeTokenRaw &raw )
{
    ::ZeroMemory( &raw, sizeof( raw ) );
    auto out = Engine::BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>(&raw), raw.size(), false );

    out->Write( client_id );
}

bool Engine::NetworkChallengeToken::Read( NetworkChallengeTokenRaw &raw )
{
    auto in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw), raw.size(), false );

    in->Read( client_id );

    in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw) + raw.size() - sizeof( authentication ), sizeof( authentication ), false );
    in->ReadBytes( authentication.data(), authentication.size() );

    return true;
}

bool Engine::NetworkChallengeToken::Decrypt( NetworkChallengeTokenRaw &raw, uint64_t sequence_num, NetworkKey &key )
{
    /* create the nonce */
    NetworkNonce nonce;
    auto nonce_alias = BitStreamFactory::CreateOutputBitStream( nonce.data(), nonce.size(), false );
    nonce_alias->Write( 0, 32 );
    nonce_alias->Write( sequence_num );

    return Networking::Encrypt( &raw, raw.size(), nullptr, 0, nonce, key );
}

bool Engine::NetworkChallengeToken::Encrypt( NetworkChallengeTokenRaw &raw, uint64_t sequence_num, NetworkKey &key )
{
    /* create the nonce */
    NetworkNonce nonce;
    auto nonce_alias = BitStreamFactory::CreateOutputBitStream( nonce.data(), nonce.size(), false );
    nonce_alias->Write( 0, 32 );
    nonce_alias->Write( sequence_num );

    return Networking::Encrypt( &raw, raw.size(), nullptr, 0, nonce, key );
}

void Engine::NetworkConnectionDeniedPacket::Write( OutputBitStreamPtr & out )
{
}
