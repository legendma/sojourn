#pragma once

#include "common/network/network_main.hpp"

#define DEFAULT_CLIENT_SOCKET_SNDBUF_SIZE ( 256 * 1024 )
#define DEFAULT_CLIENT_SOCKET_RCVBUF_SIZE ( 256 * 1024 )
#define DEFAULT_CONNECT_SEND_PERIOD_MS    ( 100 )

namespace Engine
{
    struct NetworkClientConfig
    {
        uint64_t protocol_id;
        size_t send_buff_size;
        size_t receive_buff_size;
        uint32_t connect_send_period;

        NetworkClientConfig() :
            protocol_id( NETWORK_SOJOURN_PROTOCOL_ID ),
            send_buff_size( DEFAULT_CLIENT_SOCKET_SNDBUF_SIZE ),
            receive_buff_size( DEFAULT_CLIENT_SOCKET_RCVBUF_SIZE ),
            connect_send_period( DEFAULT_CONNECT_SEND_PERIOD_MS )
        {
        };
    };
}