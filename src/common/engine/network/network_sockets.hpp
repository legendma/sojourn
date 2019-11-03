#pragma once

#include "network_address.hpp"

namespace Engine
{
    class NetworkSocketUDP
    {
        friend class NetworkSocketUDPFactory;
    public:
        ~NetworkSocketUDP();
        int Bind( const NetworkAddressPtr &from_address );
        int SendTo( const void *data_to_send, size_t length, const NetworkAddressPtr &to_address );
        int ReceiveFrom( void *data_received, size_t buffer_size, NetworkAddressPtr &came_from_address );

    private:
        NetworkSocketUDP( SOCKET &other ) : m_socket( other ) {};

        SOCKET m_socket;
    }; typedef std::shared_ptr<NetworkSocketUDP> NetworkSocketUDPPtr;

    class NetworkSocketUDPFactory
    {
    public:
        static NetworkSocketUDPPtr CreateUDPSocket( size_t receive_buffer_size = 0, size_t send_buffer_size = 0 );
        static NetworkSocketUDPPtr CreateUDPSocket( Engine::NetworkAddressPtr &our_address, size_t receive_buffer_size = 0, size_t send_buffer_size = 0 );
    };

    class NetworkSocketTCP
    {
        friend class NetworkSocketTCPFactory;
    public:
        ~NetworkSocketTCP();
        std::shared_ptr<NetworkSocketTCP> Accept( NetworkAddress &from_address );
        int Bind( const NetworkAddress &from_address );
        int Connect( const NetworkAddress &to_address );
        int Listen( int back_log = 32 );
        int Send( const void *data_to_send, size_t length );
        int Receive( void *data_received, size_t buffer_size );

    private:
        NetworkSocketTCP( SOCKET &other ) : m_socket( other ) {};

        SOCKET m_socket;
    }; typedef std::shared_ptr<NetworkSocketTCP> NetworkSocketTCPPtr;

    class NetworkSocketTCPFactory
    {
    public:
        static NetworkSocketTCPPtr CreateListenSocket( const NetworkAddress &our_address );
    };
}

