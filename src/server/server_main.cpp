// server_main.cpp : Defines the entry point for the console application.
//

#include "pch.hpp"

#include "common/network/network_main.hpp"
#include "common/engine/engine_utilities.hpp"

#include "server_main.hpp"

int wmain( int argc, wchar_t* argv[] )
{
    wprintf( L"Sojourn Server\n" );
    Engine::SetLogLevel( Engine::LOG_LEVEL_DEBUG );

    Server::ServerConfig config;
    config.server_address = std::wstring( L"localhost:48000" );
    if( argc == 2 )
    {
        config.server_address = std::wstring( argv[ 1 ] );
    }

    auto networking = Engine::NetworkingFactory::StartNetworking();
    if( networking == nullptr )
    {
        Engine::ReportError( L"Networking system could not be started.  Exiting..." );
        return -1;
    }

    auto server = Server::ServerFactory::CreateServer( config, networking );
    if( server == nullptr )
    {
        Engine::ReportError( L"Server was unable to be created.  Exiting..." );
        return -1;
    }

    server->Run();

    wprintf( L"Shutting down\n" );

    server.reset();
    networking.reset();

    return 0;
}

void Server::Server::Initialize()
{
    // Create the server address
    m_server_address = Engine::NetworkAddressFactory::CreateAddressFromStringAsync( m_config.server_address ).get();
    if( m_server_address == nullptr )
    {
        Engine::ReportError( L"Server::Initialize Given server address is invalid!" );
        throw std::runtime_error( "CreateAddressFromStringAsync" );
    }

    // Create the server socket
    m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_server_address, m_config.receive_buff_size, m_config.send_buff_size );
    if( m_socket == nullptr )
    {
        Engine::ReportError( L"Server::Initialize Unable to create server socket." );
        throw std::runtime_error( "CreateUDPSocket" );
    }

    Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server listening on %s" ).c_str(), m_server_address->Print() );

    // setup the server timer
    m_timer.SetFixedTimeStep( true );
    m_timer.SetTargetElapsedSeconds( 1.0 / m_config.server_fps );
}

Server::Server::Server( ServerConfig &config, Engine::NetworkingPtr &networking ) : 
    m_networking( networking ),
    m_quit( false ),
    m_config( config )
{
    Initialize();
}

void Server::Server::OnReceivedConnectionRequest( Engine::NetworkPacketPtr &packet )
{
    auto request = reinterpret_cast<Engine::NetworkConnectionRequestPacket&>( *packet );
    auto token = request.ReadToken();

    bool found_our_address = false;
    for( auto i = 0; i < token->server_address_cnt; i++ )
    {
        auto address = Engine::NetworkAddressFactory::CreateFromAddress( token->server_addresses[ i ] );
        if( m_server_address->Matches( *address ) )
        {
            found_our_address = true;
        }
    }

    if( !found_our_address )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  Server Address not found in whitelist." ).c_str() );
        return;
    }

    if( FindClientByAddress( request.from_address ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  Client is already connected." ).c_str() );
        return;
    }

    if( FindClientByClientID( token->client_id ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  Client with same client ID is already connected." ).c_str() );
        return;
    }

    if( !m_connection_tokens.FindAdd( token->token_uid, request.from_address, m_timer.GetElapsedSeconds() ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  A client from a different address has already used this token." ).c_str() );
        return;
    }

    if( m_clients.size() == SERVER_MAX_NUM_CLIENTS )
    {
        auto refusal = Engine::NetworkPacketFactory::CreateConnectionDenied();
        m_networking->SendPacket( m_socket, request.from_address, refusal );
        return;
    }
}

Server::Server::~Server()
{
}

void Server::Server::Run()
{
    m_timer.ResetElapsedTime();
    while( !m_quit )
    {
        m_timer.Tick( [&]()
        {
            Update();
        } );
    }
}

void Server::Server::Update()
{
    ReceivePackets();
    KeepClientsAlive(); // TODO IMPLEMENT
    CheckClientTimeouts(); // TODO IMPLEMENT
    HandleGamePacketsFromClients(); // TODO IMPLEMENT
    RunGameSimulation(); // TODO IMPLEMENT
    SendPacketsToClients(); // TODO IMPLEMENT
}

void Server::Server::ReceivePackets()
{
    Engine::NetworkPacketTypesAllowed allowed;
    allowed.SetAllowed( Engine::PACKET_CONNECT_REQUEST );
    allowed.SetAllowed( Engine::PACKET_CONNECT_CHALLENGE_RESPONSE );
    allowed.SetAllowed( Engine::PACKET_KEEP_ALIVE );
    allowed.SetAllowed( Engine::PACKET_PAYLOAD );
    allowed.SetAllowed( Engine::PACKET_DISCONNECT );

    auto now = m_timer.GetTotalTicks();
    byte data[NETWORK_MAX_PACKET_SIZE];
    while( true )
    {
        Engine::NetworkAddressPtr from;
        auto byte_cnt = m_socket->ReceiveFrom( data, sizeof( data ), from );
        if( byte_cnt == 0 )
            break;
        
        auto read = Engine::InputBitStreamFactory::CreateInputBitStream( data, byte_cnt );
        m_networking->ReadAndProcessPacket( m_config.protocol_id, allowed, from, now, read, *this );
    }
}

void Server::Server::KeepClientsAlive()
{
}

void Server::Server::CheckClientTimeouts()
{
}

void Server::Server::HandleGamePacketsFromClients()
{
}

void Server::Server::RunGameSimulation()
{
}

void Server::Server::SendPacketsToClients()
{
}

Server::ClientRecordPtr Server::Server::FindClientByAddress( Engine::NetworkAddressPtr &search )
{
    assert( m_clients.size() <= m_config.max_num_clients );
    for( auto client : m_clients )
    {
        if( client->client_address->Matches( *search ) )
        {
            return client;
        }
    }

    return nullptr;
}

Server::ClientRecordPtr Server::Server::FindClientByClientID( uint64_t search )
{
    assert( m_clients.size() <= m_config.max_num_clients );
    for( auto client : m_clients )
    {
        if( client && client->client_id == search )
        {
            return client;
        }
    }

    return nullptr;
}

Server::ServerPtr Server::ServerFactory::CreateServer( ServerConfig &config, Engine::NetworkingPtr &networking )
{
    try
    {
        return ServerPtr( new Server( config, networking ) );
    }
    catch( std::runtime_error() ) {}

    return nullptr;
}

Server::ConnectionTokens::ConnectionTokens()
{
    for( size_t i = 0; i < m_tokens.size(); i++ )
    {
        m_tokens[ i ].time = 0.0;
        memset( m_tokens[ i ].token_uid.data(), 0, sizeof( Engine::NetworkKey ) );
    }
}

bool Server::ConnectionTokens::FindAdd( Engine::NetworkAuthentication &token_uid, Engine::NetworkAddressPtr address, double time )
{
    /* first search for the token id in our entries */
    EntryType *match = nullptr;
    EntryType *oldest = nullptr;
    for( size_t i = 0; i < m_tokens.size(); i++ )
    {
        if( 0 == std::memcmp( m_tokens[ i ].token_uid.data(), token_uid.data(), sizeof( Engine::NetworkKey ) ) )
        {
            match = &m_tokens[ i ];
        }

        if( !oldest
         || m_tokens[ i ].time < oldest->time )
        {
            oldest = &m_tokens[ i ];
        }
    }

    if( !match )
    {
        /* didn't find a match for the given token UID, so overwrite the oldest entry with this token */
        oldest->address = address;
        oldest->time = time;
        std::memcpy( oldest->token_uid.data(), token_uid.data(), sizeof( Engine::NetworkKey ) );
        return true;
    }

    /* check to make sure the token entry we found has come from the same address */
    if( match->address->Matches( *address ) )
    {
        return true;
    }

    return false;
}
