#include "pch.hpp"

#include "engine_utilities.hpp"

#include "network_socket_tcp.hpp"

Engine::NetworkSocketTCP::~NetworkSocketTCP()
{
    closesocket( m_socket );
}

Engine::NetworkSocketTCPPtr Engine::NetworkSocketTCP::Accept( NetworkAddress &from_address )
{
    int from_length = static_cast<int>( from_address.GetSize() );
    auto new_socket = accept( m_socket, &from_address.m_address, &from_length );
    if( new_socket == INVALID_SOCKET )
    {
        Engine::ReportError( L"NetworkSocketTCP::Accept" );
        return nullptr;
    }

    return Engine::NetworkSocketTCPPtr( new NetworkSocketTCP( new_socket ) );
}

int Engine::NetworkSocketTCP::Bind( const NetworkAddress &from_address )
{
    auto result = bind( m_socket, &from_address.m_address, static_cast<int>( from_address.GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketTCP::Bind" );
        return -WSAGetLastError();
    }
    return NO_ERROR;
}

int Engine::NetworkSocketTCP::Connect( const NetworkAddress &to_address )
{
    auto result = connect( m_socket, &to_address.m_address, static_cast<int>(to_address.GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketTCP::Connect" );
        return -WSAGetLastError();
    }

    return NO_ERROR;
}

int Engine::NetworkSocketTCP::Listen( int back_log )
{
    auto result = listen( m_socket, back_log );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketTCP::Listen" );
        return -WSAGetLastError();
    }

    return 0;
}

int Engine::NetworkSocketTCP::Send( const void *data_to_send, size_t length )
{
    auto result = send( m_socket, static_cast<PCSTR>( data_to_send ), static_cast<int>( length ), 0 );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketTCP::Send" );
        // return negative of error code, so it's not mistaken for bytes sent
        return -WSAGetLastError();
    }

    // Result is the number of bytes sent
    return result;
}

int Engine::NetworkSocketTCP::Receive( void *data_received, size_t buffer_size )
{
    auto result = recv( m_socket, static_cast<PSTR>( data_received ), static_cast<int>( buffer_size ), 0 );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketTCP::Receive" );
        // return negative of error code, so it's not mistaken for bytes received
        return -WSAGetLastError();
    }

    // Result is the number of bytes received into the buffer
    return result;
}

Engine::NetworkSocketTCPPtr Engine::NetworkSocketTCPFactory::CreateListenSocket( const NetworkAddress &our_address )
{
    auto s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( s == INVALID_SOCKET )
    {
        Engine::ReportError( L"NetworkSocketTCPFactory::CreateListenSocket" );
        return(nullptr);
    }

    // create the new listen socket and try to bind it
    auto tcp = NetworkSocketTCPPtr( new NetworkSocketTCP( s ) );
    auto result = tcp->Bind( our_address );
    if( result != NO_ERROR )
    {
        Engine::ReportError( L"NetworkSocketTCPFactory::CreateListenSocket" );
        return( nullptr );
    }

    return tcp;
}
