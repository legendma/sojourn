#pragma once

#include "common/engine/engine_step_timer.hpp"
#include "common/network/network_main.hpp"
#include "common/network/network_sockets.hpp"
#include "common/network/network_address.hpp"


#define DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_FRAMES_PER_SEC     ( 60 )
#define DEFAULT_SERVER_MAX_CLIENT_CNT     ( 8 )

namespace Server
{
    struct ClientRecord
    {
        Engine::NetworkAddressPtr m_client_address;
    };

    typedef std::shared_ptr<ClientRecord> ClientRecordPtr;

    struct ServerConfig
    {
        uint64_t protocol_id;
        uint8_t private_key[NETWORK_PRIVATE_KEY_BYTE_CNT];
        size_t send_buff_size;
        size_t receive_buff_size;
        int server_fps;
        std::wstring server_address;
        unsigned int max_num_clients;

        ServerConfig() : 
            protocol_id( NETWORK_SOJOURN_PROTOCOL_ID ),
            send_buff_size( DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ),
            receive_buff_size( DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ),
            server_fps( DEFAULT_SERVER_FRAMES_PER_SEC ),
            max_num_clients( DEFAULT_SERVER_MAX_CLIENT_CNT )
        {};
    };

    class Server : public Engine::IProcessesPackets
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
        std::vector<ClientRecordPtr> m_clients;
        
        void Update();
        void ReceivePackets();
        void KeepClientsAlive();
        void CheckClientTimeouts();
        void HandleGamePacketsFromClients();
        void RunGameSimulation();
        void SendPacketsToClients();
        ClientRecordPtr FindClientByAddress( Engine::NetworkAddressPtr & search );
        void Initialize();
        Server( ServerConfig &config, Engine::NetworkingPtr &networking );

        // IProcessesPackets interface
        virtual void OnReceivedConnectionRequest( Engine::NetworkPacketPtr &packet );
    };

    typedef std::shared_ptr<Server> ServerPtr;

    class ServerFactory
    {
    public:
        static ServerPtr CreateServer( ServerConfig &config, Engine::NetworkingPtr &networking );
    };
}