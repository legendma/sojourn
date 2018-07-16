#pragma once

#include "common/engine/engine_step_timer.hpp"
#include "common/network/network_sockets.hpp"
#include "common/network/network_address.hpp"

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
#define MAX_PACKET_SIZE                   ( 1200 )
#define DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_FRAMES_PER_SEC     ( 60 )



namespace Server
{
    struct ServerConfig
    {
        uint64_t protocol_id;
        uint8_t private_key[PRIVATE_KEY_BYTE_CNT];
        INetworkAllocation *allocator;
        INetworkCallback *callback;
        size_t send_buff_size;
        size_t receive_buff_size;
        int server_fps;
        std::wstring server_address;

        ServerConfig() : protocol_id( SOJOURN_PROTOCOL_ID ),
            allocator( nullptr ),
            callback( nullptr ),
            send_buff_size( DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ),
            receive_buff_size( DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ),
            server_fps( DEFAULT_SERVER_FRAMES_PER_SEC )
        {};
    };

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
        Engine::StepTimer m_timer;
        boolean m_quit;
        ServerConfig m_config;
        
        void Update();
        void ReceivePackets();
        void ReadAndProcessPacket( Engine::PacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, byte *packet, int byte_cnt, uint64_t now );
        void Initialize();
        Server( ServerConfig &config, Engine::NetworkingPtr &networking );
    };

    typedef std::shared_ptr<Server> ServerPtr;

    class ServerFactory
    {
    public:
        static ServerPtr CreateServer( ServerConfig &config, Engine::NetworkingPtr &networking );
    };
}