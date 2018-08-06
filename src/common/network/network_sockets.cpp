#include "pch.hpp"

#include "common/engine/engine_utilities.hpp"

#include "network_sockets.hpp"

Engine::NetworkSocketUDP::~NetworkSocketUDP()
{
    auto result = closesocket( m_socket );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketUDP::~NetworkSocketUDP" );
    }
}

int Engine::NetworkSocketUDP::Bind( const NetworkAddressPtr &from_address )
{
    auto result = bind( m_socket, &from_address->m_address, static_cast<int>( from_address->GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketUDP::Bind" );
        return -WSAGetLastError();
    }

    return NO_ERROR;
}

int Engine::NetworkSocketUDP::SendTo( const void *data_to_send, size_t length, const NetworkAddressPtr &to_address )
{
    auto result = sendto( m_socket, static_cast<PCSTR>( data_to_send ), static_cast<int>( length ), 0, &to_address->m_address, static_cast<int>( to_address->GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketUDP::SendTo" );
        // return negative of error code, so it's not mistaken for bytes sent
        return -WSAGetLastError();
    }

    // Result is the number of bytes sent
    return result;
}

int Engine::NetworkSocketUDP::ReceiveFrom( void *data_received, size_t buffer_size, NetworkAddressPtr &came_from_address )
{
    sockaddr from;
    int from_length = sizeof( from );
    auto result = recvfrom( m_socket, static_cast<PSTR>( data_received ), static_cast<int>( buffer_size ), 0, &from, &from_length );
    if( result == SOCKET_ERROR )
    {
        auto error = WSAGetLastError();
        if( error == WSAEWOULDBLOCK
         || error == WSAECONNRESET )
        {
            return 0;
        }

        Engine::ReportWinsockError( L"NetworkSocketUDP::ReceiveFrom", error );
        Engine::Log( LOG_LEVEL_ERROR, L"Error code: %d", error );
        return 0;
    }

    // Result is the number of bytes received into the buffer
    came_from_address = NetworkAddressPtr( new NetworkAddress( from ) );
    return result;
}

Engine::NetworkSocketUDPPtr Engine::NetworkSocketUDPFactory::CreateUDPSocket( size_t receive_buffer_size, size_t send_buffer_size )
{
    auto s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( s == INVALID_SOCKET )
    {
        Engine::ReportWinsockError( L"NetworkSocketUDPFactory::CreateUDPSocket failed to create a new socket" );
        return( nullptr );
    }

    // try to configure the send and receive buffer sizes
    if( receive_buffer_size > 0 )
    {
        auto result = setsockopt( s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<LPCSTR>( &receive_buffer_size ), sizeof( receive_buffer_size ) );
        if( result != NO_ERROR )
        {
            Engine::ReportWinsockError( L"NetworkSocketUDPFactory::CreateUDPSocket failed to set new recieve buffer size" );
            closesocket( s );
            return nullptr;
        }
    }

    if( send_buffer_size > 0 )
    {
        auto result = setsockopt( s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<LPCSTR>(&send_buffer_size), sizeof( send_buffer_size ) );
        if( result != NO_ERROR )
        {
            Engine::ReportWinsockError( L"NetworkSocketUDPFactory::CreateUDPSocket failed to set new send buffer size" );
            closesocket( s );
            return nullptr;
        }
    }

    // set as non-blocking IO
    u_long dont_block = TRUE;
    auto result = ioctlsocket( s, FIONBIO, &dont_block );
    if( result != NO_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketUDPFactory::CreateUDPSocket failed to set socket as non-blocking" );
        closesocket( s );
        return nullptr;
    }

    return NetworkSocketUDPPtr( new NetworkSocketUDP( s ) );
}

Engine::NetworkSocketUDPPtr Engine::NetworkSocketUDPFactory::CreateUDPSocket( Engine::NetworkAddressPtr &our_address, size_t receive_buffer_size, size_t send_buffer_size )
{
    auto new_socket = NetworkSocketUDPFactory::CreateUDPSocket( receive_buffer_size, send_buffer_size );
    if( new_socket == nullptr )
    {
        return nullptr;
    }
    
    auto result = new_socket->Bind( our_address );
    if( result != NO_ERROR )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"NetworkSocketUDPFactory::CreateUDPSocket failed to bind to the address and port" );
        return nullptr;
    }

    return new_socket;
}

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
        Engine::ReportWinsockError( L"NetworkSocketTCP::Accept" );
        return nullptr;
    }

    return Engine::NetworkSocketTCPPtr( new NetworkSocketTCP( new_socket ) );
}

int Engine::NetworkSocketTCP::Bind( const NetworkAddress &from_address )
{
    auto result = bind( m_socket, &from_address.m_address, static_cast<int>( from_address.GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketTCP::Bind" );
        return -WSAGetLastError();
    }
    return NO_ERROR;
}

int Engine::NetworkSocketTCP::Connect( const NetworkAddress &to_address )
{
    auto result = connect( m_socket, &to_address.m_address, static_cast<int>(to_address.GetSize() ) );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketTCP::Connect" );
        return -WSAGetLastError();
    }

    return NO_ERROR;
}

int Engine::NetworkSocketTCP::Listen( int back_log )
{
    auto result = listen( m_socket, back_log );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketTCP::Listen" );
        return -WSAGetLastError();
    }

    return 0;
}

int Engine::NetworkSocketTCP::Send( const void *data_to_send, size_t length )
{
    auto result = send( m_socket, static_cast<PCSTR>( data_to_send ), static_cast<int>( length ), 0 );
    if( result == SOCKET_ERROR )
    {
        Engine::ReportWinsockError( L"NetworkSocketTCP::Send" );
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
        Engine::ReportWinsockError( L"NetworkSocketTCP::Receive" );
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
        Engine::ReportWinsockError( L"NetworkSocketTCPFactory::CreateListenSocket" );
        return(nullptr);
    }

    // create the new listen socket and try to bind it
    auto tcp = NetworkSocketTCPPtr( new NetworkSocketTCP( s ) );
    auto result = tcp->Bind( our_address );
    if( result != NO_ERROR )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"NetworkSocketTCPFactory::CreateListenSocket" );
        return( nullptr );
    }

    return tcp;
}
