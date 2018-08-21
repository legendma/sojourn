#pragma once

#include "network_platform.hpp"
#include "network_sockets.hpp"
#include "network_address.hpp"
#include "network_types.hpp"
#include "network_buffers.hpp"
                                             
#define NETWORK_PRIVATE_KEY_BYTE_CNT         ( 32 )
#define NETWORK_SOJOURN_PROTOCOL_ID          ( 0xda47091958c38397 )
#define NETWORK_MAX_PACKET_SIZE              ( 1200 )
#define NETWORK_PROTOCOL_VERSION_LEN         ( 16 )
#define NETWORK_PROTOCOL_VERSION             ( "Sojourn Ver 1.0\0" )
#define NETWORK_CONNECT_TOKEN_RAW_LENGTH     ( 1024 )
#define NETWORK_CHALLENGE_TOKEN_RAW_LENGTH   ( 400 )
#define NETWORK_MESSAGE_DATA_RAW_LENGTH      ( 1100 )
#define NETWORK_FUZZ_LENGTH                  ( 300 )
#define NETCODE_MAX_SERVERS_PER_CONNECT      ( 32 )
#define NETWORK_NUM_CRYPO_MAPS               ( 1024 )
                                             
#define NETWORK_PACKET_TYPE_BITS             ( 4 )
#define NETWORK_SEQUENCE_NUM_BITS            ( 4 )

namespace Engine
{
    typedef std::array<byte, NETWORK_CONNECT_TOKEN_RAW_LENGTH> NetworkConnectionTokenRaw;
    struct NetworkConnectionToken
    {
        uint64_t client_id;
        int32_t timeout_seconds;
        int32_t server_address_cnt;
        std::array<sockaddr, NETCODE_MAX_SERVERS_PER_CONNECT> server_addresses;
        NetworkKey client_to_server_key;
        NetworkKey server_to_client_key;
        NetworkFuzz fuzz;
        NetworkAuthentication authentication;

        void Write( NetworkConnectionTokenRaw &raw );
        bool Read( NetworkConnectionTokenRaw &raw );
        static bool Decrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, double expire_time, const NetworkKey &key );
        static bool Encrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, double expire_time, const NetworkKey &key );
    }; typedef std::shared_ptr<NetworkConnectionToken> NetworkConnectionTokenPtr;

    typedef std::array<byte, NETWORK_CHALLENGE_TOKEN_RAW_LENGTH> NetworkChallengeTokenRaw;
    struct NetworkChallengeToken
    {
        uint64_t client_id;
        NetworkFuzz fuzz;
        NetworkAuthentication authentication;

        void Write( NetworkChallengeTokenRaw &raw );
        bool Read( NetworkChallengeTokenRaw &raw );
        static bool Decrypt( NetworkChallengeTokenRaw &raw, uint64_t sequence_num, const NetworkKey &key );
        static bool Encrypt( NetworkChallengeTokenRaw &raw, uint64_t sequence_num, NetworkKey &key );
    }; typedef std::shared_ptr<NetworkChallengeToken> NetworkChallengeTokenPtr;

    typedef std::array<byte, NETWORK_MESSAGE_DATA_RAW_LENGTH> NetworkMessageDataRaw;

#pragma pack(push, 1)
    union NetworkPacketPrefix
    {
        struct
        {
            byte packet_type : NETWORK_PACKET_TYPE_BITS; /* NetworkPacketType */
            byte sequence_byte_cnt : NETWORK_SEQUENCE_NUM_BITS;
        };

        uint8_t b;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct NetworkConnectionRequestHeader
    {
        std::array<char, NETWORK_PROTOCOL_VERSION_LEN> version;
        uint64_t protocol_id;
        double token_expire_time;
        uint64_t token_sequence;
        NetworkConnectionTokenRaw raw_token;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct NetworkConnectionChallengeHeader
    {
        uint64_t token_sequence;
        NetworkChallengeTokenRaw raw_challenge_token;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct NetworkConnectionChallengeResponseHeader
    {
        uint64_t token_sequence;
        NetworkChallengeTokenRaw raw_challenge_token;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct NetworkKeepAliveHeader
    {
        uint64_t client_id;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct NetworkPayloadHeader
    {
        uint64_t client_id;
        uint16_t packet_ack_recent_sequence;
        uint32_t packet_ack_sequence_bits;
        NetworkMessageDataRaw message_data;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct NetworkDisconnectHeader
    {
        NetworkPacketPrefix prefix;
    };
#pragma pack(pop)
    
    class NetworkPacket;
    typedef std::shared_ptr<NetworkPacket> NetworkPacketPtr;
    class NetworkPacket
    {
    public:
        NetworkPacketType packet_type;

        static NetworkPacketPtr ReadPacket( InputBitStreamPtr &read, NetworkPacketTypesAllowed &allowed, uint64_t protocol_id, NetworkKey &read_key, double now_time );
        OutputBitStreamPtr WritePacket( uint64_t sequence_number, uint64_t protocol_id, NetworkKey &key );

    private:
        virtual void Write( OutputBitStreamPtr &out ) = 0;
    };

    class NetworkConnectionRequestPacket : public NetworkPacket
    {
        friend class NetworkPacketFactory;
    public:
        NetworkConnectionRequestHeader header;
        NetworkConnectionTokenPtr token;

        static NetworkPacketPtr Read( InputBitStreamPtr &in, uint64_t &protocol_id, double now_time );

    private:
        virtual void Write( OutputBitStreamPtr &out );
        NetworkConnectionRequestPacket() { packet_type = PACKET_CONNECT_REQUEST; }
    };

    class NetworkConnectionDeniedPacket : public NetworkPacket
    {
        friend class NetworkPacketFactory;
    public:
        NetworkConnectionRequestHeader header;
        NetworkConnectionTokenPtr token;

        static NetworkPacketPtr Read( InputBitStreamPtr &in );

    private:
        virtual void Write( OutputBitStreamPtr &out ) {};
        NetworkConnectionDeniedPacket() { packet_type = PACKET_CONNECT_DENIED; }
    };

    class NetworkConnectionChallengePacket : public NetworkPacket
    {
        friend class NetworkPacketFactory;
    public:
        NetworkConnectionChallengeHeader header;
        NetworkChallengeTokenPtr token;

        static NetworkPacketPtr Read( InputBitStreamPtr &in );

    private:
        virtual void Write( OutputBitStreamPtr &out );
        NetworkConnectionChallengePacket() { packet_type = PACKET_CONNECT_CHALLENGE; }
    };

    class NetworkConnectionChallengeResponsePacket : public NetworkPacket
    {
        friend class NetworkPacketFactory;
    public:
        NetworkConnectionChallengeResponseHeader header;
        NetworkChallengeTokenPtr token;

        static NetworkPacketPtr Read( InputBitStreamPtr &in );

    private:
        virtual void Write( OutputBitStreamPtr &out );
        NetworkConnectionChallengeResponsePacket() { packet_type = PACKET_CONNECT_CHALLENGE_RESPONSE; }
    };

    class NetworkKeepAlivePacket : public NetworkPacket
    {
        friend class NetworkPacketFactory;
    public:
        NetworkKeepAliveHeader header;

        static NetworkPacketPtr Read( InputBitStreamPtr &in );

    private:
        virtual void Write( OutputBitStreamPtr &out );
        NetworkKeepAlivePacket() { packet_type = PACKET_KEEP_ALIVE; }
    };

    class NetworkDisconnectPacket : public NetworkPacket
    {
        friend class NetworkPacketFactory;
    public:
        static NetworkPacketPtr Read( InputBitStreamPtr &in );

    private:
        virtual void Write( OutputBitStreamPtr &out ) {};
        NetworkDisconnectPacket() { packet_type = PACKET_DISCONNECT; }
    };

    class NetworkPayloadPacket : public NetworkPacket
    {
        friend class NetworkPacketFactory;
    public:
        NetworkPayloadHeader header;
        uint16_t sequence_num;
        int message_bytes;

        static NetworkPacketPtr Read( InputBitStreamPtr &in, uint16_t sequence_num );

    private:
        virtual void Write( OutputBitStreamPtr &out );
        NetworkPayloadPacket() { packet_type = PACKET_PAYLOAD; message_bytes = 0; sequence_num = 0; }
    };

    class NetworkPacketFactory
    {
    public:
        static NetworkPacketPtr CreateConnectionRequest( NetworkConnectionRequestHeader &header, NetworkConnectionTokenPtr token = nullptr );
        static NetworkPacketPtr CreateConnectionDenied();
        static NetworkPacketPtr CreateConnectionChallenge( NetworkConnectionChallengeHeader &header );
        static NetworkPacketPtr CreateConnectionChallengeResponse( NetworkConnectionChallengeResponseHeader &header );
        static NetworkPacketPtr CreateDisconnect();
        static NetworkPacketPtr CreateKeepAlive( NetworkKeepAliveHeader &header );
        static NetworkPacketPtr CreateKeepAlive( uint64_t client_id );
        static NetworkPacketPtr CreatePayload( NetworkPayloadHeader &header, uint16_t sequence_num, int message_bytes );
    };

    class NetworkCryptoMap
    {
    public:
        NetworkCryptoMap() {};
        
        NetworkAddressPtr address;
        double last_seen;
        double expire_time;
        int timeout_seconds;
        NetworkKey send_key;
        NetworkKey receive_key;

        bool IsExpired( double current_time )
        {
            if( timeout_seconds > 0 )
            {
                if( last_seen + timeout_seconds < current_time )
                {
                    /* timed out */
                    return true;
                }

                return false;
            }

            /* check expiration */
            return ( expire_time > 0.0 && expire_time < current_time );
        }

    }; typedef std::shared_ptr<NetworkCryptoMap> NetworkCryptoMapPtr;

    class Networking
    {
        friend class NetworkingFactory;
    public:
        ~Networking();

        bool SendPacket( Engine::NetworkSocketUDPPtr &socket, NetworkAddressPtr &to, NetworkPacketPtr &packet, uint64_t protocol_id, NetworkKey &key, uint64_t sequence_num );
        void AddCryptoMap( uint64_t client_id, NetworkAddressPtr &client_address, NetworkKey &send_key, NetworkKey &receive_key, double now_time, double expire_time, int timeout_secs );
        bool DeleteCryptoMapsFromAddress( NetworkAddressPtr &address );
        NetworkCryptoMapPtr FindCryptoMapByAddress( NetworkAddressPtr &search_address, double time );
        NetworkCryptoMapPtr FindCryptoMapByClientID( uint64_t search_id, NetworkAddressPtr &expected_address, double time );

        static bool Encrypt( void *data_to_encrypt, size_t data_length, byte* salt, size_t salt_length, NetworkNonce &nonce, const NetworkKey &key );
        static bool Decrypt( void *data_to_decrypt, size_t data_length, byte *salt, size_t salt_length, NetworkNonce &nonce, const NetworkKey &key );
        static void GenerateEncryptionKey( NetworkKey &key );
        static void GenerateRandom( byte *out, size_t size );

    private:
        WSADATA m_wsa_data;
        std::map<uint64_t, NetworkCryptoMapPtr> m_crypto_map;

        Networking();
        void Initialize();
    }; typedef std::shared_ptr<Networking> NetworkingPtr;

    class NetworkingFactory
    {
    public:
        static NetworkingPtr StartNetworking();
    };
}

