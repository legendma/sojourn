#include "pch.hpp"

#include "engine_utilities.hpp"

#include "network_socket_udp.hpp"

Engine::NetworkSocketUDP::~NetworkSocketUDP()
{
    closesocket( m_socket );
}

int Engine::NetworkSocketUDP::Bind( const NetworkAddress &from_address )
{
    auto result = bind( m_socket, &from_address.m_address, static_cast<int>( from_address.GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketUDP::Bind" );
        return WSAGetLastError();
    }
    return NO_ERROR;
}

int Engine::NetworkSocketUDP::SendTo( const void *data_to_send, size_t length, const NetworkAddress &to_address )
{
    auto result = sendto( m_socket, static_cast<PCSTR>( data_to_send ), static_cast<int>( length ), 0, &to_address.m_address, static_cast<int>( to_address.GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketUDP::SendTo" );
        // return negative of error code, so it's not mistaken for bytes sent
        return -WSAGetLastError();
    }

    // Result is the number of bytes sent
    return result;
}

int Engine::NetworkSocketUDP::ReceiveFrom( void *data_received, size_t buffer_size, NetworkAddress &came_from_address )
{
    int from_length = static_cast<int>(came_from_address.GetSize() );
    auto result = recvfrom( m_socket, static_cast<PSTR>( data_received ), static_cast<int>( buffer_size ), 0, &came_from_address.m_address, &from_length );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportError( L"NetworkSocketUDP::ReceiveFrom" );
        // return negative of error code, so it's not mistaken for bytes received
        return -WSAGetLastError();
    }

    // Result is the number of bytes received into the buffer
    return result;
}

Engine::NetworkSocketUDPPtr Engine::NetworkSocketUDPFactory::CreateUDPSocket()
{
    auto s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( s == INVALID_SOCKET )
    {
        Engine::ReportError( L"NetworkSocketUDPFactory::CreateUDPSocket" );
        return( nullptr );
    }

    return NetworkSocketUDPPtr( new NetworkSocketUDP( s ) );
}
