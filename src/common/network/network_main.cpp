#include "pch.hpp"
#include "network_main.hpp"

#include "common/engine/engine_utilities.hpp"

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
    // Start WinSock
    auto result = WSAStartup( MAKEWORD( 2, 2 ), &m_wsa_data );
    if( result != NO_ERROR )
    {
        Engine::ReportError( L"Networking::Initialize could not initialize Winsock DLL." );
        throw std::runtime_error( "WSAStartup" );
    }

}

Engine::NetworkingPtr Engine::NetworkingFactory::CreateNetworking()
{
    try
    {
        return ( NetworkingPtr( new Networking() ) );
    }
    catch( std::runtime_error() ) {}
      
    return( nullptr );
}
