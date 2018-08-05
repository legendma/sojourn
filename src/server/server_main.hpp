#pragma once

#include "common/engine/engine_step_timer.hpp"
#include "common/network/network_main.hpp"
#include "common/network/network_sockets.hpp"
#include "common/network/network_address.hpp"
#include "common/network/network_matchmaking.hpp"


#define DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_FRAMES_PER_SEC     ( 60 )
#define DEFAULT_SERVER_MAX_CLIENT_CNT     ( 8 )

#define SERVER_MAX_CONNECT_TOKENS         ( 2000 )

namespace Server
{
    struct ClientRecord
    {
        Engine::NetworkAddressPtr client_address;
        uint64_t client_id;
        bool is_confirmed;
        double last_time_received_packet;
        double last_time_sent_packet;
        int timeout_seconds;

        ClientRecord() :
            client_id( 0 ),
            last_time_received_packet( 0.0 ),
            last_time_sent_packet( 0.0 ),
            timeout_seconds( 0 ),
            is_confirmed( false ) {};
    }; typedef std::shared_ptr<ClientRecord> ClientRecordPtr;

    struct ServerConfig
    {
        uint64_t protocol_id;
        size_t send_buff_size;
        size_t receive_buff_size;
        int server_fps;
        std::wstring server_address;
        unsigned int max_num_clients;
        Engine::NetworkKey challenge_key;

        ServerConfig() : 
            protocol_id( NETWORK_SOJOURN_PROTOCOL_ID ),
            send_buff_size( DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ),
            receive_buff_size( DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ),
            server_fps( DEFAULT_SERVER_FRAMES_PER_SEC ),
            max_num_clients( DEFAULT_SERVER_MAX_CLIENT_CNT )
        {
            Engine::Networking::GenerateEncryptionKey( challenge_key );
        };
    };

    struct SeenTokens
    {
        struct EntryType
        {
            Engine::NetworkAuthentication token_uid;
            Engine::NetworkAddressPtr address;
            double time;
        }; typedef std::shared_ptr<EntryType> EntryTypePtr;

        SeenTokens();
        EntryTypePtr FindAdd( Engine::NetworkAuthentication &token_uid, Engine::NetworkAddressPtr address, double time );

    private:
        std::array<EntryType, SERVER_MAX_CONNECT_TOKENS> m_seen;
        
    };

    class Server
    {
        friend class ServerFactory;
    public:
        ~Server();
        void Run();

    private:
        Engine::NetworkingPtr m_networking;
        Engine::NetworkAddressPtr m_server_address;
        Engine::NetworkSocketUDPPtr m_socket;
        Engine::StepTimer m_timer;
        boolean m_quit;
        ServerConfig m_config;
        std::vector<ClientRecordPtr> m_clients;
        SeenTokens m_seen_tokens;
        uint64_t m_next_challenge_sequence;
        uint64_t m_next_sequence;
        double m_now_time;
        
        void Update();
        void ReadAndProcessPacket( uint64_t protocol_id, Engine::NetworkPacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, Engine::InputBitStreamPtr &read );
        void ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from );
        void ReceivePackets();
        void KeepClientsAlive();
        void CheckClientTimeouts();
        void HandleGamePacketsFromClients();
        void RunGameSimulation();
        void SendPacketsToClients();
        ClientRecordPtr FindClientByAddress( Engine::NetworkAddressPtr &search );
        ClientRecordPtr FindClientByClientID( uint64_t search );
        void Initialize();
        Server( ServerConfig &config, Engine::NetworkingPtr &networking );

        void OnReceivedConnectionRequest( Engine::NetworkConnectionRequestPacket &request, Engine::NetworkAddressPtr &from );
        void OnReceivedConnectionChallengeResponse( Engine::NetworkConnectionChallengeResponsePacket &response, Engine::NetworkAddressPtr &from );
        void OnReceivedKeepAlive( Engine::NetworkKeepAlivePacket &keep_alive, Engine::NetworkAddressPtr &from );
    }; typedef std::shared_ptr<Server> ServerPtr;

    class ServerFactory
    {
    public:
        static ServerPtr CreateServer( ServerConfig &config, Engine::NetworkingPtr &networking );
    };
}