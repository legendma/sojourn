#pragma once

#include "common/engine/network/network_main.hpp"

#define DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )
#define DEFAULT_SERVER_FRAMES_PER_SEC     ( 60 )
#define DEFAULT_SERVER_MAX_CLIENT_CNT     ( 8 )
#define DEFAULT_CONNECT_SEND_PERIOD_MS    ( 100 )

namespace Server
{
    struct NetworkServerConfig
    {
        uint64_t protocol_id;
        size_t send_buff_size;
        size_t receive_buff_size;
        int server_fps;
        std::wstring server_address;
        unsigned int max_num_clients;
        Engine::NetworkKey challenge_key;
        double send_rate;

        NetworkServerConfig() :
            protocol_id( NETWORK_SOJOURN_PROTOCOL_ID ),
            send_buff_size( DEFAULT_SERVER_SOCKET_SNDBUF_SIZE ),
            receive_buff_size( DEFAULT_SERVER_SOCKET_RCVBUF_SIZE ),
            server_fps( DEFAULT_SERVER_FRAMES_PER_SEC ),
            max_num_clients( DEFAULT_SERVER_MAX_CLIENT_CNT ),
            send_rate( DEFAULT_CONNECT_SEND_PERIOD_MS )
        {
            Engine::Networking::GenerateEncryptionKey( challenge_key );
        };
    };
}