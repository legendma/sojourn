#pragma once

#include "common/network/network_socket_udp.hpp"

class ClientProxy
{

};

interface INetworkAllocation
{
    virtual void * Allocate( size_t allocation_size ) = 0;
    virtual void Free( void *pointer ) = 0;
};

interface INetworkCallback
{
    virtual void ConnectDisconnect( ClientProxy &client );

};

#define PRIVATE_KEY_BYTE_CNT              ( 32 )
#define SOJOURN_PROTOCOL_ID               ( 0x1ceb00dacafebabe )
#define DEFAULT_MAX_PACKET_SIZE           ( 1200 )
#define DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )

#include "common/network/network_address.hpp"

struct ServerConfig
{
    uint64_t protocol_id;
    uint8_t private_key[PRIVATE_KEY_BYTE_CNT];
    INetworkAllocation *allocator;
    INetworkCallback *callback;
    size_t max_packet_size;
    size_t send_buff_size;
    size_t receive_buff_size;
    std::wstring server_address;

    ServerConfig() : protocol_id( SOJOURN_PROTOCOL_ID ),
                     allocator( nullptr ),
                     callback( nullptr ),
                     max_packet_size( DEFAULT_MAX_PACKET_SIZE ),
                     send_buff_size( DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ),
                     receive_buff_size( DEFAULT_SERVER_SOCKET_RCVBUF_SIZE )
                     {};
};

namespace Server
{
    class Server
    {
        friend class ServerFactory;
    public:
        ~Server();
        void Run();

    private:
        //std::vector<byte> m_packet_data;
        Engine::NetworkingPtr m_networking;
        Engine::NetworkAddressPtr m_server_address;
        Engine::NetworkSocketUDPPtr m_socket;
        
        void Initialize( ServerConfig &config );
        Server( ServerConfig &config, Engine::NetworkingPtr &networking );
    };

    typedef std::shared_ptr<Server> ServerPtr;

    class ServerFactory
    {
    public:
        static ServerPtr CreateServer( ServerConfig &config, Engine::NetworkingPtr &networking );
    };
}