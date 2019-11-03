#pragma once

#include "engine/network/network_server_config.hpp"
#include "common/engine/network/network_main.hpp"
#include "common/engine/network/network_reliable_endpoint.hpp"
#include "common/engine/engine_step_timer.hpp"
#include "common/game/game_simulation.hpp"

#define SERVER_NUM_OF_DISCONNECT_PACKETS  ( 10 )
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
        uint64_t client_sequence;
        Engine::NetworkReliableEndpointPtr endpoint;

        ClientRecord() :
            client_id( 0 ),
            last_time_received_packet( 0.0 ),
            last_time_sent_packet( 0.0 ),
            timeout_seconds( 0 ),
            client_sequence( 0 ),
            is_confirmed( false ) {};
    }; typedef std::shared_ptr<ClientRecord> ClientRecordPtr;

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

    class Application
    {
    public:
        Application( std::wstring server_address );

        bool Start();
        int Run();
        void Shutdown();

    protected:
        void CheckClientTimeouts();
        void DisconnectClient( uint64_t client_id, int num_of_disconnect_packets = SERVER_NUM_OF_DISCONNECT_PACKETS );
        ClientRecordPtr FindClientByAddress( Engine::NetworkAddressPtr &search );
        ClientRecordPtr FindClientByClientID( uint64_t search );
        void HandleGamePacketsFromClients();
        void KeepClientsAlive();
        void OnReceivedConnectionChallengeResponse( Engine::NetworkConnectionChallengeResponsePacket &response, Engine::NetworkAddressPtr &from );
        void OnReceivedConnectionRequest( Engine::NetworkConnectionRequestPacket &request, Engine::NetworkAddressPtr &from );
        void OnReceivedKeepAlive( Engine::NetworkKeepAlivePacket &keep_alive, ClientRecordPtr &client );
        void ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, ClientRecordPtr &client );
        void ReadAndProcessPacket( uint64_t protocol_id, Engine::NetworkPacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, Engine::InputBitStreamPtr &read );
        void ReceivePackets();
        void RunGameSimulation();
        bool SendClientPacket( uint64_t client_id, Engine::NetworkPacketPtr &packet );
        void SendGamePacketsToClients();

    private:
        std::wstring m_server_address_string;
        Engine::NetworkAddressPtr m_server_address;
        NetworkServerConfig m_config;
        std::vector<ClientRecordPtr> m_clients;
        Engine::NetworkSocketUDPPtr m_socket;
        SeenTokens m_seen_tokens;
        Engine::StepTimer m_timer;
        double m_now_time;
        boolean m_quit;
        uint64_t m_next_challenge_sequence;
        uint64_t m_next_sequence;

        Game::GameSimulationPtr m_simulation;
        Engine::NetworkingPtr m_networking;
    };

    static const wchar_t *SPLASH = L"\n"
        L"     .d8888b.            d8b                                         \n"
        L"     d88P  Y88b           Y8P                                        \n"
        L"     Y88b.                                                           \n"
        L"      `Y888b.    .d88b.  8888  .d88b.  888  888 888d888 88888b.      \n"
        L"         `Y88b. d88`'88b `888 d88'`88b 888  888 888P'   888 `88b     \n"
        L"           `888 888  888  888 888  888 888  888 888     888  888     \n"
        L"     Y88b  d88P Y88..88P  888 Y88..88P Y88b 888 888     888  888     \n"
        L"      `Y8888P'   `Y88P'   888  `Y88P'   `Y88888 888     888  888     \n"
        L"                          888                                        \n"
        L"            .d8888b.     d88P                                        \n"
        L"           d88P  Y88b  888P'                                         \n"
        L"           Y88b.                                                     \n"
        L"            `Y888b.    .d88b.  888d888 888  888  .d88b.  888d888     \n"
        L"               `Y88b. d8P  Y8b 888P'   888  888 d8P  Y8b 888P'       \n"
        L"                 `888 88888888 888     Y88  88P 88888888 888         \n"
        L"           Y88b  d88P Y8b.     888      Y8bd8P  Y8b.     888         \n"
        L"            `Y8888P'   `Y8888  888       Y88P    `Y8888  888         \n\n"
        L"---------------------------------------------------------------------\n\n";
}