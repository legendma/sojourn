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

    auto server_address = std::wstring( L"localhost:48000" );
    if( argc == 2 )
    {
        server_address = std::wstring( argv[ 1 ] );
    }

    auto networking = Engine::NetworkingFactory::CreateNetworking();
    if( networking == nullptr )
    {
        Engine::ReportError( L"Networking system could not be started.  Exiting..." );
        return -1;
    }

    Server::ServerConfig config;
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
}

void Server::Server::ReceivePackets()
{
    Engine::PacketTypesAllowed allowed;
    allowed.SetAllowed( Engine::PACKET_REQUEST );
    allowed.SetAllowed( Engine::PACKET_RESPONSE );
    allowed.SetAllowed( Engine::PACKET_KEEP_ALIVE );
    allowed.SetAllowed( Engine::PACKET_PAYLOAD );
    allowed.SetAllowed( Engine::PACKET_DISCONNECT );

    auto now = m_timer.GetTotalTicks();
    while( true )
    {
        byte data[ MAX_PACKET_SIZE ];
        Engine::NetworkAddressPtr from;
        auto byte_cnt = m_socket->ReceiveFrom( data, sizeof( data ), from );
        if( byte_cnt == 0 )
        {
            break;
        }

        ReadAndProcessPacket( allowed, from, data, byte_cnt, now );
    }
}

void Server::Server::ReadAndProcessPacket( Engine::PacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, byte *packet, int byte_cnt, uint64_t now )
{

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
