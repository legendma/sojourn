#include "pch.hpp"

#include "common/engine/engine_utilities.hpp"

#include "network_main.hpp"
#include "network_matchmaking.hpp"

#define NETWORK_SYSTEM_MEMORY_SIZE ( 1024 * 1014 / 2 )

Engine::Networking::Networking()
{
    Initialize();
}

Engine::Networking::~Networking()
{
    WSACleanup();
}

void Engine::Networking::Initialize()
{
    m_allocator = std::shared_ptr<IMemoryAllocator>( new MemorySystem( NETWORK_SYSTEM_MEMORY_SIZE ) );

    /* start WinSock */
    auto result = WSAStartup( MAKEWORD( 2, 2 ), &m_wsa_data );
    if( result != NO_ERROR )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Networking::Initialize could not initialize Winsock DLL." );
        throw std::runtime_error( "WSAStartup" );
    }
}

bool Engine::Networking::SendPacket( Engine::NetworkSocketUDPPtr &socket, NetworkAddressPtr &to, NetworkPacketPtr &packet, uint64_t protocol_id, NetworkKey &key, uint64_t sequence_num )
{
    auto buffer = packet->WritePacket( sequence_num, protocol_id, key );
    if( !buffer )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Networking::SendPacket unable to write packet." );
        return false;
    }
    assert( buffer->GetCurrentByteCount() < NETWORK_MAX_PACKET_SIZE );

    if( socket->SendTo( buffer->GetBuffer(), buffer->GetCurrentByteCount(), to ) < 0 )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Networking::SendPacket unable to send packet to destination." );
        return false;
    }

    return true;
}

void Engine::Networking::AddCryptoMap( uint64_t client_id, NetworkAddressPtr &client_address, NetworkKey &send_key, NetworkKey &receive_key, double now_time, double expire_time, int timeout_secs )
{
    auto mapping = m_crypto_map.begin();
    while( mapping != m_crypto_map.end() )
    {
        auto crypto = mapping->second;
        if( mapping->second->IsExpired( now_time ) )
        {
            mapping = m_crypto_map.erase( mapping );
            continue;
        }

        if( crypto->address->Matches( *client_address )
          && mapping->first == client_id )
        {
            /* found an existing mapping from this client, so just update it */
            crypto->expire_time = expire_time;
            crypto->timeout_seconds = timeout_secs;
            crypto->last_seen = now_time;
            crypto->send_key = send_key;
            crypto->receive_key = receive_key;            

            return;
        }
    }

    /* create a new record */
    auto new_record = NetworkCryptoMapPtr( new NetworkCryptoMap() );
    m_crypto_map.insert( std::make_pair( client_id, new_record ) );

    new_record->address = client_address;
    new_record->expire_time = expire_time;
    new_record->timeout_seconds = timeout_secs;
    new_record->last_seen = now_time;
    new_record->send_key = send_key;
    new_record->receive_key = receive_key;

}

bool Engine::Networking::DeleteCryptoMapsFromAddress( NetworkAddressPtr &address )
{
    bool found = false;
    auto mapping = m_crypto_map.begin();
    while( mapping != m_crypto_map.end() )
    {
        auto crypto = mapping->second;
        if( crypto->address->Matches( *address ) )
        {
            found = true;
            mapping = m_crypto_map.erase( mapping );
            continue;
        }

        mapping++;
    }

    return found;
}

Engine::NetworkCryptoMapPtr Engine::Networking::FindCryptoMapByAddress( NetworkAddressPtr &search_address, double time )
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

Engine::NetworkCryptoMapPtr Engine::Networking::FindCryptoMapByClientID( uint64_t search_id, NetworkAddressPtr &expected_address, double time )
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

Engine::MemoryAllocatorPtr Engine::Networking::AsAllocator()
{
    return m_allocator;
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
                                                             nullptr, data, static_cast<unsigned long long>(data_length),
                                                             salt, salt_length,
                                                             nonce.data(), key.data() );

    if( result != 0 )
    {
        return false;
    }

    assert( decrypted_length == data_length - sizeof( NetworkAuthentication ) );
    return true;
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

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateConnectionRequest( MemoryAllocatorPtr allocator, NetworkConnectionRequestHeader &header, NetworkConnectionTokenPtr token )
{
    auto ptr = allocator->Allocate( sizeof( NetworkConnectionRequestPacket ), L"NetworkPacketFactory::CreateConnectionRequest" );
    auto packet = std::shared_ptr<NetworkConnectionRequestPacket>( new( ptr ) NetworkConnectionRequestPacket(), [allocator]( NetworkConnectionRequestPacket *p )
    {
        allocator->Free( p );
    } );

    packet->header = header;

    if( token )
    {
        packet->token = token;
    }
    else
    {
        auto token = allocator->Allocate( sizeof( NetworkConnectionToken ), L"NetworkPacketFactory::CreateConnectionRequest" );
        packet->token = std::shared_ptr<NetworkConnectionToken>( new(token) NetworkConnectionToken(), [allocator]( NetworkConnectionToken *p )
        {
            allocator->Free( p );
        } );
    }

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateConnectionDenied( MemoryAllocatorPtr allocator )
{
    auto ptr = allocator->Allocate( sizeof( NetworkConnectionDeniedPacket ), L"NetworkPacketFactory::CreateConnectionDenied" );
    auto packet = std::shared_ptr<NetworkConnectionDeniedPacket>( new(ptr) NetworkConnectionDeniedPacket(), [allocator]( NetworkConnectionDeniedPacket *p )
    {
        allocator->Free( p );
    } );

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateConnectionChallenge( MemoryAllocatorPtr allocator, NetworkConnectionChallengeHeader &header )
{
    auto ptr = allocator->Allocate( sizeof( NetworkConnectionChallengePacket ), L"NetworkPacketFactory::CreateConnectionChallenge" );
    auto packet = std::shared_ptr<NetworkConnectionChallengePacket>( new(ptr) NetworkConnectionChallengePacket(), [allocator]( NetworkConnectionChallengePacket *p )
    {
        allocator->Free( p );
    } );
    
    packet->header = header;

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateConnectionChallengeResponse( MemoryAllocatorPtr allocator, NetworkConnectionChallengeResponseHeader &header )
{
    auto ptr = allocator->Allocate( sizeof( NetworkConnectionChallengeResponsePacket ), L"NetworkPacketFactory::CreateConnectionChallengeResponse" );
    auto packet = std::shared_ptr<NetworkConnectionChallengeResponsePacket>( new(ptr) NetworkConnectionChallengeResponsePacket(), [allocator]( NetworkConnectionChallengeResponsePacket *p )
    {
        allocator->Free( p );
    } );

    packet->header = header;
    packet->token = NetworkChallengeTokenPtr( new NetworkChallengeToken() );

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateDisconnect( MemoryAllocatorPtr allocator )
{
    auto ptr = allocator->Allocate( sizeof( NetworkDisconnectPacket ), L"NetworkPacketFactory::CreateDisconnect" );
    auto packet = std::shared_ptr<NetworkDisconnectPacket>( new(ptr) NetworkDisconnectPacket(), [allocator]( NetworkDisconnectPacket *p )
    {
        allocator->Free( p );
    } );

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateKeepAlive( MemoryAllocatorPtr allocator, NetworkKeepAliveHeader &header )
{
    auto ptr = allocator->Allocate( sizeof( NetworkKeepAlivePacket ), L"NetworkPacketFactory::CreateKeepAlive" );
    auto packet = std::shared_ptr<NetworkKeepAlivePacket>( new(ptr) NetworkKeepAlivePacket(), [allocator]( NetworkKeepAlivePacket *p )
    {
        allocator->Free( p );
    } );

    packet->header = header;

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateKeepAlive( MemoryAllocatorPtr allocator, uint64_t client_id )
{
    auto ptr = allocator->Allocate( sizeof( NetworkKeepAlivePacket ), L"NetworkPacketFactory::CreateKeepAlive" );
    auto packet = std::shared_ptr<NetworkKeepAlivePacket>( new(ptr) NetworkKeepAlivePacket(), [allocator]( NetworkKeepAlivePacket *p )
    {
        allocator->Free( p );
    } );

    packet->header.client_id = client_id;

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreatePayload( MemoryAllocatorPtr allocator, NetworkPayloadHeader &header, int message_bytes )
{
    auto ptr = allocator->Allocate( sizeof( NetworkPayloadPacket ), L"NetworkPacketFactory::CreatePayload" );
    auto packet = std::shared_ptr<NetworkPayloadPacket>( new(ptr) NetworkPayloadPacket(), [allocator]( NetworkPayloadPacket *p )
    {
        allocator->Free( p );
    } );

    packet->header = header;
    packet->message_bytes = message_bytes;

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacket::ReadPacket( MemoryAllocatorPtr allocator, InputBitStreamPtr &read, NetworkPacketTypesAllowed &allowed, uint64_t protocol_id, NetworkKey &read_key, double now_time )
{
    Engine::NetworkPacketPrefix prefix;
    read->Write( prefix.b );
    if( !allowed.IsAllowed( static_cast<NetworkPacketType>(prefix.packet_type) ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored packet.  Type not allowed." );
        return nullptr;
    }

    /* non-encrypted connect request */
    if( prefix.packet_type == Engine::PACKET_CONNECT_REQUEST )
    {
        return Engine::NetworkConnectionRequestPacket::Read( allocator, read, protocol_id, now_time );
    }

    /* encrypted packet type */
    if( prefix.sequence_byte_cnt < 1
     || prefix.sequence_byte_cnt > 8 )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Packet.  Sequence bytes of %d is not within bounds.", prefix.sequence_byte_cnt );
        return nullptr;
    }

    if( read->GetSize() < sizeof( NetworkPacketPrefix ) + prefix.sequence_byte_cnt + sizeof( NetworkAuthentication ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Packet.  Packet smaller than size for empty packet.", prefix.sequence_byte_cnt );
        return nullptr;
    }

    /* read the sequence number */
    uint64_t sequence_number = 0;
    read->Write( sequence_number, prefix.sequence_byte_cnt * 8 );

    /* create the nonce */
    NetworkNonce nonce;
    auto nonce_alias = BitStreamFactory::CreateOutputBitStream( nonce.data(), nonce.size(), false );
    nonce_alias->Write( 0, 32 );
    nonce_alias->Write( sequence_number );

    /* create the salt */
    struct
    {
        byte version[NETWORK_PROTOCOL_VERSION_LEN];
        uint64_t protocol_id;
        NetworkPacketPrefix prefix;
    } salt;

    auto salt_alias = BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>(&salt), sizeof( salt ), false );
    salt_alias->WriteBytes( (void*)NETWORK_PROTOCOL_VERSION, NETWORK_PROTOCOL_VERSION_LEN );
    salt_alias->Write( protocol_id );
    salt_alias->Write( prefix.b );

    if( !Networking::Decrypt( read->GetBufferAtCurrent(), read->GetRemainingByteCount(), salt_alias->GetBuffer(), salt_alias->GetCurrentByteCount(), nonce, read_key ) )
    {
        return nullptr;
    }

    /* read the packet by packet type */
    switch( prefix.packet_type )
    {
    case PACKET_CONNECT_DENIED:
        return NetworkConnectionDeniedPacket::Read( allocator, read );

    case PACKET_CONNECT_CHALLENGE:
        return NetworkConnectionChallengePacket::Read( allocator, read );

    case PACKET_CONNECT_CHALLENGE_RESPONSE:
        return NetworkConnectionChallengeResponsePacket::Read( allocator, read );

    case PACKET_KEEP_ALIVE:
        return NetworkKeepAlivePacket::Read( allocator, read );

    case PACKET_PAYLOAD:
        return NetworkPayloadPacket::Read( allocator, read );

    case PACKET_DISCONNECT:
        return NetworkDisconnectPacket::Read( allocator, read );

    default:
        break;
    }

    return nullptr;
}

Engine::OutputBitStreamPtr Engine::NetworkPacket::WritePacket( uint64_t sequence_number, uint64_t protocol_id, NetworkKey &key )
{
    auto out = BitStreamFactory::CreateOutputBitStream();
   
    /* handle connection requests without encryption */
    if( packet_type == PACKET_CONNECT_REQUEST )
    {
        NetworkPacketPrefix prefix;
        prefix.packet_type = packet_type;
        prefix.sequence_byte_cnt = 0;
        out->Write( prefix.b );
        Write( out );
        return out;
    }

    /* encrypted packet */
    auto sequence_byte_cnt = BitStreamBase::BytesRequired( sequence_number );

    /* write the unencrypted section */
    NetworkPacketPrefix prefix;
    prefix.packet_type = packet_type;
    prefix.sequence_byte_cnt = sequence_byte_cnt;
    out->Write( prefix.b );
    out->Write( sequence_number, sequence_byte_cnt * 8 );

    /* write the data to be encrypted */
    auto encrypted = BitStreamFactory::CreateOutputBitStream();
    Write( encrypted );

    /* create the nonce */
    NetworkNonce nonce;
    auto nonce_alias = BitStreamFactory::CreateOutputBitStream( nonce.data(), nonce.size(), false );
    nonce_alias->Write( 0, 32 );
    nonce_alias->Write( sequence_number );

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

    NetworkAuthentication authentication;
    encrypted->Write( authentication );

    /* encrypt and append to the output packet buffer */
    if( !Networking::Encrypt( encrypted->GetBuffer(), encrypted->GetCurrentByteCount(), salt_alias->GetBuffer(), salt_alias->GetCurrentByteCount(), nonce, key ) )
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
    out->WriteBytes( reinterpret_cast<byte*>(&header.raw_token), sizeof( NetworkConnectionTokenRaw ) );
}

Engine::NetworkPacketPtr Engine::NetworkConnectionRequestPacket::Read( MemoryAllocatorPtr allocator, InputBitStreamPtr &in, uint64_t &protocol_id, double now_time )
{
    if( in->GetRemainingByteCount() != sizeof( Engine::NetworkConnectionRequestHeader ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Request.  Bad packet size.  Expected %d, got %d.", sizeof( Engine::NetworkConnectionRequestHeader ), in->GetSize() );
        return nullptr;
    }

    Engine::NetworkConnectionRequestHeader request;
    ::ZeroMemory( &request, sizeof( request ) );

    /* test if protocol version matches */
    in->WriteBytes( &request.version, request.version.size() );
    if( std::string( request.version.data() ) != std::string( NETWORK_PROTOCOL_VERSION ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Request.  Protocol version was incorrect." );
        return nullptr;
    }

    /* test if protocol ID matches */
    in->Write( request.protocol_id );
    if( request.protocol_id != protocol_id )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Request.  Invalid Protocol ID." );
        return nullptr;
    }

    /* test if the connection token has expired */
    in->Write( request.token_expire_time );
    if( now_time > request.token_expire_time )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Request.  Connection token expired." );
        return nullptr;
    }

    /* decrypt and read the connection token */
    in->Write( request.token_sequence );

    in->WriteBytes( &request.raw_token, sizeof( request.raw_token ) );
    if( !NetworkConnectionToken::Decrypt( request.raw_token, request.token_sequence, protocol_id, request.token_expire_time, Engine::SOJOURN_PRIVILEGED_KEY ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Request.  Could not decrypt." );
        return nullptr;
    }

    NetworkConnectionTokenPtr plain_token( new NetworkConnectionToken() );
    if( !plain_token->Read( request.raw_token ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Request.  Could not read connection token." );
        return nullptr;
    }

    return Engine::NetworkPacketFactory::CreateConnectionRequest( allocator, request, plain_token );
}

bool Engine::NetworkConnectionToken::Read( NetworkConnectionTokenRaw &raw )
{
    auto in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw), raw.size(), false );

    in->Write( client_id );
    in->Write( timeout_seconds );
    in->Write( server_address_cnt );

    if( server_address_cnt <= 0
     || server_address_cnt > (int)server_addresses.size() )
    {
        return false;
    }

    for( auto i = 0; i < server_address_cnt; i++ )
    {
        in->Write( server_addresses[i] );
    }

    in->Write( client_to_server_key );
    in->Write( server_to_client_key );

    in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw) + raw.size() - sizeof(authentication), sizeof( authentication ), false );
    in->WriteBytes( authentication.data(), authentication.size() );

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

bool Engine::NetworkConnectionToken::Encrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, double expire_time, const NetworkKey &key )
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
        double expire_time;
    } salt;

    auto salt_alias = BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>(&salt), sizeof( salt ), false );
    salt_alias->WriteBytes( (void*)NETWORK_PROTOCOL_VERSION, NETWORK_PROTOCOL_VERSION_LEN );
    salt_alias->Write( protocol_id );
    salt_alias->Write( expire_time );

    return Networking::Encrypt( &raw, raw.size(), salt_alias->GetBuffer(), salt_alias->GetCurrentByteCount(), nonce, key );
}

bool Engine::NetworkConnectionToken::Decrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, double expire_time, const NetworkKey &key )
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
        double expire_time;
    } salt;

    auto salt_alias = BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>(&salt), sizeof( salt ), false );
    salt_alias->WriteBytes( (void*)NETWORK_PROTOCOL_VERSION, NETWORK_PROTOCOL_VERSION_LEN );
    salt_alias->Write( protocol_id );
    salt_alias->Write( expire_time );

    return Networking::Decrypt( &raw, raw.size(), salt_alias->GetBuffer(), salt_alias->GetCurrentByteCount(), nonce, key );
}

Engine::NetworkPacketPtr Engine::NetworkConnectionChallengePacket::Read( MemoryAllocatorPtr allocator, InputBitStreamPtr &in )
{
    if( in->GetRemainingByteCount() != sizeof( NetworkConnectionChallengeHeader ) + sizeof( NetworkAuthentication ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Challenge.  Bad packet size.  Expected %d, got %d.", sizeof( NetworkAuthentication ), in->GetSize() );
        return nullptr;
    }

    Engine::NetworkConnectionChallengeHeader challenge;
    ::ZeroMemory( &challenge, sizeof( challenge ) );

    /* read the encrypted challenge token for later decryption */
    in->Write( challenge.token_sequence );
    in->WriteBytes( &challenge.raw_challenge_token, sizeof( challenge.raw_challenge_token ) );

    return Engine::NetworkPacketFactory::CreateConnectionChallenge( allocator, challenge );
}

void Engine::NetworkConnectionChallengePacket::Write( OutputBitStreamPtr &out )
{
    out->Write( header.token_sequence );
    out->WriteBytes( header.raw_challenge_token.data(), header.raw_challenge_token.size() );
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

    in->Write( client_id );

    in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw) + raw.size() - sizeof( authentication ), sizeof( authentication ), false );
    in->WriteBytes( authentication.data(), authentication.size() );

    return true;
}

bool Engine::NetworkChallengeToken::Decrypt( NetworkChallengeTokenRaw &raw, uint64_t sequence_num, const NetworkKey &key )
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

Engine::NetworkPacketPtr Engine::NetworkConnectionChallengeResponsePacket::Read( MemoryAllocatorPtr allocator, InputBitStreamPtr &in )
{
    if( in->GetRemainingByteCount() != sizeof( NetworkConnectionChallengeResponseHeader ) + sizeof( NetworkAuthentication ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Response.  Bad packet size.  Expected %d, got %d.", sizeof( NetworkConnectionChallengeResponseHeader ) + sizeof( NetworkAuthentication ), in->GetSize() );
        return nullptr;
    }

    Engine::NetworkConnectionChallengeResponseHeader response;
    ::ZeroMemory( &response, sizeof( response ) );

    in->Write( response.token_sequence );
    in->WriteBytes( response.raw_challenge_token.data(), response.raw_challenge_token.size() );

    return NetworkPacketFactory::CreateConnectionChallengeResponse( allocator, response );
}

void Engine::NetworkConnectionChallengeResponsePacket::Write( OutputBitStreamPtr &out )
{
    out->Write( header.token_sequence );
    out->WriteBytes( header.raw_challenge_token.data(), header.raw_challenge_token.size() );
}

Engine::NetworkPacketPtr Engine::NetworkConnectionDeniedPacket::Read( MemoryAllocatorPtr allocator, InputBitStreamPtr &in )
{
    if( in->GetRemainingByteCount() != sizeof( NetworkAuthentication ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Denied.  Bad packet size.  Expected %d, got %d.", sizeof( NetworkAuthentication ), in->GetSize() );
        return nullptr;
    }

    return NetworkPacketFactory::CreateConnectionDenied( allocator );
}

Engine::NetworkPacketPtr Engine::NetworkKeepAlivePacket::Read( MemoryAllocatorPtr allocator, InputBitStreamPtr & in )
{
    if( in->GetRemainingByteCount() != sizeof( NetworkKeepAliveHeader ) + sizeof( NetworkAuthentication ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Connection Denied.  Bad packet size.  Expected 0, got %d.", sizeof( NetworkAuthentication ) + sizeof( NetworkAuthentication ), in->GetSize() );
        return nullptr;
    }

    NetworkKeepAliveHeader keep_alive;
    ::ZeroMemory( &keep_alive, sizeof( keep_alive ) );

    in->Write( keep_alive.client_id );

    return NetworkPacketFactory::CreateKeepAlive( allocator, keep_alive );
}

void Engine::NetworkKeepAlivePacket::Write( OutputBitStreamPtr &out )
{
    out->Write( header.client_id );
}

Engine::NetworkPacketPtr Engine::NetworkDisconnectPacket::Read( MemoryAllocatorPtr allocator, InputBitStreamPtr & in )
{
    if( in->GetRemainingByteCount() != sizeof( NetworkAuthentication ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Ignored Disconnect.  Bad packet size.  Expected %d, got %d.", sizeof( NetworkAuthentication ), in->GetSize() );
        return nullptr;
    }

    return NetworkPacketFactory::CreateDisconnect( allocator );
}

Engine::NetworkPacketPtr Engine::NetworkPayloadPacket::Read( MemoryAllocatorPtr allocator, InputBitStreamPtr &in )
{
    NetworkPayloadHeader header;
    in->Write( header.client_id );
    in->Write( header.sequence );
    in->Write( header.packet_ack_recent_sequence );
    in->Write( header.packet_ack_sequence_bits );
    in->Write( header.start_message );

    auto message_bytes = in->GetRemainingByteCount();
    in->WriteBytes( header.message_data.data(), message_bytes );

    auto packet = NetworkPacketFactory::CreatePayload( allocator, header, message_bytes );
    auto payload = reinterpret_cast<NetworkPayloadPacket&>( *packet );

    return packet;
}

void Engine::NetworkPayloadPacket::Write( OutputBitStreamPtr &out )
{
    out->Write( header.client_id );
    out->Write( header.sequence );
    out->Write( header.packet_ack_recent_sequence );
    out->Write( header.packet_ack_sequence_bits );

    out->WriteBytes( header.message_data.data(), message_bytes );
}
