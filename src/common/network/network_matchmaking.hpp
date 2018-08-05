#pragma once

#include "network_main.hpp"

#define NETWORK_CONNECT_PASSPORT_RAW_LENGTH     ( 1800 )

namespace Engine
{
    static const NetworkKey SOJOURN_PRIVILEGED_KEY = { 0x56, 0xd7, 0xd2, 0x1d, 0x50, 0x4c, 0x5b, 0x76,
                                                       0x99, 0x09, 0xf7, 0xdf, 0xce, 0x6e, 0x58, 0x4e,
                                                       0x6b, 0xa1, 0x58, 0xcd, 0x24, 0x71, 0x4d, 0xae,
                                                       0x44, 0x25, 0xfa, 0xe9, 0x6b, 0xc6, 0x59, 0xa4 };

    typedef std::array<byte, NETWORK_CONNECT_PASSPORT_RAW_LENGTH> NetworkConnectionPassportRaw;
    struct NetworkConnectionPassport
    {
        std::array<char, NETWORK_PROTOCOL_VERSION_LEN> version;
        uint64_t protocol_id;
        double token_create_time;
        double token_expire_time;
        uint64_t token_sequence;
        int32_t timeout_seconds;
        int32_t server_address_cnt;
        std::array<sockaddr, NETCODE_MAX_SERVERS_PER_CONNECT> server_addresses;
        NetworkKey client_to_server_key;
        NetworkKey server_to_client_key;
        NetworkConnectionTokenRaw raw_token;

        int current_server;

        void Write( NetworkConnectionPassportRaw &raw );
        bool Read( NetworkConnectionPassportRaw &raw );
    }; typedef std::shared_ptr<NetworkConnectionPassport> NetworkConnectionPassportPtr;
    
    bool FAKE_NetworkGetPassport( NetworkConnectionPassportRaw &out );
}

