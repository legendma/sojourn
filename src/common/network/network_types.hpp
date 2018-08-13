#pragma once
                                            
#define NETWORK_KEY_LENGTH                   ( crypto_aead_chacha20poly1305_IETF_KEYBYTES )
#define NETWORK_AUTHENTICATION_LENGTH        ( crypto_aead_chacha20poly1305_IETF_ABYTES )
#define NETWORK_NONCE_LENGTH                 ( crypto_aead_chacha20poly1305_IETF_NPUBBYTES )
#define NETWORK_FUZZ_LENGTH                  ( 300 )

namespace Engine
{
    typedef std::array<byte, NETWORK_KEY_LENGTH> NetworkKey;
    typedef std::array<byte, NETWORK_AUTHENTICATION_LENGTH> NetworkAuthentication;
    typedef std::array<byte, NETWORK_NONCE_LENGTH> NetworkNonce;
    typedef std::array<byte, NETWORK_FUZZ_LENGTH> NetworkFuzz;

    typedef enum
    {
        PACKET_CONNECT_REQUEST,
        PACKET_CONNECT_DENIED,
        PACKET_CONNECT_CHALLENGE,
        PACKET_CONNECT_CHALLENGE_RESPONSE,
        PACKET_KEEP_ALIVE,
        PACKET_PAYLOAD,
        PACKET_DISCONNECT,
        PACKET_TYPE_CNT,
        PACKET_INVALID = PACKET_TYPE_CNT
    } NetworkPacketType;

    struct NetworkPacketTypesAllowed
    {
        boolean allowed[PACKET_TYPE_CNT];

        inline void SetAllowed( const NetworkPacketType packet_type ) { allowed[packet_type] = true; }
        inline boolean IsAllowed( const NetworkPacketType packet_type ) { return(allowed[packet_type]); }
        inline void Reset() { ::ZeroMemory( allowed, sizeof( allowed ) ); }

        NetworkPacketTypesAllowed() { Reset(); }
    };

}

