#include "pch.hpp"
#include "network_main.hpp"

#include "common/engine/engine_utilities.hpp"

Engine::Networking::Networking() :
    m_sequence_num( 0 )
{
    Initialize();
}

Engine::Networking::~Networking()
{
    WSACleanup();
}

void Engine::Networking::Initialize()
{
    // Start WinSock
    auto result = WSAStartup( MAKEWORD( 2, 2 ), &m_wsa_data );
    if( result != NO_ERROR )
    {
        Engine::ReportError( L"Networking::Initialize could not initialize Winsock DLL." );
        throw std::runtime_error( "WSAStartup" );
    }

}

void Engine::Networking::ReadAndProcessPacket( uint64_t protocol_id, Engine::NetworkPacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, uint64_t now_time, Engine::InputBitStreamPtr &read, IProcessesPackets &processor )
{
    auto client = nullptr;//FindClientByAddress( from );
    if( client )
    {
        // TODO: Get the encryption data
    }
    else
    {
        // TODO: Create new encryption data
    }

    /*
    -----------------------------------------------
    | packet_type 4-bits   | sequence_num 4-bits  |
    -----------------------------------------------
    */
    NetworkPacketPtr packet;
    byte packet_type;
    read->ReadBits( packet_type, NETWORK_PACKET_TYPE_BITS );
    if( packet_type == Engine::PACKET_CONNECT_REQUEST )
    {
        /* handle new connection request */
        read->Advance( NETWORK_SEQUENCE_NUM_BITS );

        Engine::NetworkConnectionRequestHeader request;
        ::ZeroMemory( &request, sizeof(request) );
        request.prefix.packet_type = packet_type;

        if( !allowed.IsAllowed( Engine::PACKET_CONNECT_REQUEST ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request from %s.  Not allowed." ).c_str(), from->Print() );
            return;
        }
        else if( read->GetSize() != sizeof( Engine::NetworkConnectionRequestHeader ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request from %s.  Bad packet size.  Expected %d, got %d." ).c_str(), from->Print(), sizeof( Engine::NetworkConnectionRequestHeader ), read->GetSize() );
        }
        // TODO: Add check for private key

        /* test if protocol version matches */
        read->ReadBytes( &request.version, request.version.size() );
        if( std::string( request.version.data() ) != std::string( NETWORK_PROTOCOL_VERSION ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request from %s.  Protocol version was incorrect." ).c_str(), from->Print() );
            return;
        }

        /* test if protocol ID matches */
        read->ReadBytes( &request.protocol_id, sizeof( request.protocol_id ) );
        if( request.protocol_id != protocol_id )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request from %s.  Invalid Protocol ID." ).c_str(), from->Print() );
            return;
        }

        /* test if the connection token has expired */
        read->ReadBytes( &request.connection_token_expiration_timestamp, sizeof( request.connection_token_expiration_timestamp ) );
        if( now_time > request.connection_token_expiration_timestamp )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Ignored Connection Request from %s.  Connection token expired." ).c_str(), from->Print() );
            return;
        }

        packet = Engine::NetworkPacketFactory::CreateConnectionRequest( request, from );
    }

    ProcessPacket( processor, packet );
}

void Engine::Networking::ProcessPacket( IProcessesPackets &process, NetworkPacketPtr &packet )
{
    switch( packet->packet_type )
    {
    case Engine::PACKET_CONNECT_REQUEST:
        process.OnReceivedConnectionRequest( packet );
        break;
    }
}

void Engine::Networking::SendPacket( Engine::NetworkSocketUDPPtr &socket, NetworkAddressPtr &to, NetworkPacketPtr &packet )
{
    auto buffer = packet->Get( m_sequence_num++ );
    socket->SendTo( buffer->GetBuffer(), buffer->GetSize(), to );
}

bool Engine::Networking::Encrypt( byte *data_to_encrypt, size_t data_length, byte *salt, size_t salt_length, byte *nonce, NetworkKey key )
{
    unsigned long long encrypted_length;
    byte *buffer = static_cast<byte*>( alloca( data_length + NETWORK_AUTHENTICATION_LENGTH ) );
    crypto_aead_chacha20poly1305_ietf_encrypt( buffer, &encrypted_length,
                                               data_to_encrypt, static_cast<unsigned long long>(data_length),
                                               salt, salt_length,
                                               NULL, nonce, key.data() );
        
    return true;
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
        ReallocateBuffer( max( m_bit_capacity * 2, next_bit_head ) );
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

void Engine::OutputBitStream::Write( sockaddr &out )
{
    Write( out.sa_family );
    WriteBytes( out.sa_data, sizeof( out.sa_data ) );
}

size_t Engine::OutputBitStream::Collapse()
{
    size_t size = ( 7 + m_bit_head ) / 8;
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

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateConnectionRequest( NetworkConnectionRequestHeader &header, NetworkAddressPtr &from )
{
    auto packet = std::make_shared<NetworkConnectionRequestPacket>();
    packet->header = header;
    packet->from_address = from;

    return packet;
}

Engine::NetworkPacketPtr Engine::NetworkPacketFactory::CreateConnectionDenied()
{
    auto packet = std::make_shared<NetworkConnectionRequestPacket>();
    
    packet->header.prefix.packet_type = PACKET_CONNECT_DENIED;
    packet->header.prefix.sequence_byte_cnt = 0;

    return packet;
}

Engine::NetworkConnectionTokenPtr Engine::NetworkConnectionRequestPacket::ReadToken()
{
    auto token = std::make_shared<NetworkConnectionToken>();

    auto read = InputBitStreamFactory::CreateInputBitStream( header.connection_token_data.data(), header.connection_token_data.size() );
    
    read->ReadBytes( &token->client_id, sizeof( token->client_id ) );
    read->ReadBytes( &token->timeout_seconds, sizeof( token->timeout_seconds) );
    read->ReadBytes( &token->server_address_cnt, sizeof( token->server_address_cnt ) );

    if( token->server_address_cnt <= 0
    || token->server_address_cnt > NETCODE_MAX_SERVERS_PER_CONNECT )
    {
        return nullptr;
    }

    for( int i = 0; i < token->server_address_cnt; i++ )
    {
        read->Read( token->server_addresses[ i ] );
    }

    read->Read( token->client_to_server_key );
    read->Read( token->server_to_client_key );
    read->ReadBytes( token->token_uid.data(), NETWORK_AUTHENTICATION_LENGTH );

    return token;
}

void Engine::NetworkConnectionRequestPacket::Write( OutputBitStreamPtr &out )
{
    out->Write( header.prefix.b );
    out->WriteBytes( header.version.data(), header.version.size() );
    out->Write( header.protocol_id );
    out->Write( header.connection_token_expiration_timestamp );
    out->Write( header.connection_token_sequence );
    out->WriteBytes( header.connection_token_data.data(), header.connection_token_data.size() );
}

Engine::OutputBitStreamPtr Engine::NetworkPacket::Get( uint64_t sequence_num )
{
    auto out = OutputBitStreamFactory::CreateOutputBitStream();
   
    /* handle connection requests without encryption */
    if( packet_type == PACKET_CONNECT_REQUEST )
    {
        Write( out );
        out->Collapse();
        return out;
    }

    /* encrypted packet */
    auto sequence_byte_cnt = ( 7 + BitStreamBase::BitsRequired( sequence_num ) ) / 8;

    NetworkPacketPrefix prefix;
    prefix.packet_type = packet_type;
    prefix.sequence_byte_cnt = sequence_byte_cnt;
    out->Write( prefix.b );
    out->Write( sequence_num, sequence_byte_cnt * 8 );

    auto encrypted = OutputBitStreamFactory::CreateOutputBitStream();
    Write( encrypted );
    
    return out;
}
