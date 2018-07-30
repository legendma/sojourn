#pragma once

#include "common/network/network_platform.hpp"
#include "common/network/network_sockets.hpp"
#include "common/network/network_address.hpp"

#define NETWORK_ENDIANNESS                   LITTLE_ENDIAN
                                             
#define NETWORK_PRIVATE_KEY_BYTE_CNT         ( 32 )
#define NETWORK_SOJOURN_PROTOCOL_ID          ( 0x1ceb00dacafebabe )
#define NETWORK_MAX_PACKET_SIZE              ( 1200 )
#define NETWORK_PROTOCOL_VERSION_LEN         ( 16 )
#define NETWORK_PROTOCOL_VERSION             ( "Sojourn Ver 1.0\0" )
#define NETWORK_CONNECT_TOKEN_RAW_LENGTH     ( 1024 )
#define NETWORK_CHALLENGE_TOKEN_RAW_LENGTH   ( 250 )
#define NETCODE_MAX_SERVERS_PER_CONNECT      ( 32 )
#define NETWORK_NUM_CRYPO_MAPS               ( 1024 )
                                             
#define NETWORK_PACKET_TYPE_BITS             ( 4 )
#define NETWORK_SEQUENCE_NUM_BITS            ( 4 )
                                             
#define NETWORK_KEY_LENGTH                   ( crypto_aead_chacha20poly1305_IETF_KEYBYTES )
#define NETWORK_AUTHENTICATION_LENGTH        ( crypto_aead_chacha20poly1305_IETF_ABYTES )
#define NETWORK_NONCE_LENGTH                 ( crypto_aead_chacha20poly1305_IETF_NPUBBYTES )

namespace Engine
{
    typedef std::array<byte, NETWORK_KEY_LENGTH> NetworkKey;
    typedef std::array<byte, NETWORK_AUTHENTICATION_LENGTH> NetworkAuthentication;
    typedef std::array<byte, NETWORK_NONCE_LENGTH> NetworkNonce;

    class BitStreamBase
    {
    public:
        BitStreamBase( bool is_owned );
        ~BitStreamBase() { std::free( m_buffer ); }

        byte * GetBuffer() { return m_buffer; }
        static int BitsRequired( uint64_t value );
        static int BytesRequired( uint64_t value );
        virtual size_t GetSize() = 0;
        
    protected:
        byte     *m_buffer;
        uint32_t  m_bit_head;
        uint32_t  m_bit_capacity;
        bool      m_owned;

        void ReallocateBuffer( const size_t size );
        void BindBuffer( byte* buffer, const size_t size );
    };

    class InputBitStream : public BitStreamBase
    {
        friend class InputBitStreamFactory;
    public:
        ~InputBitStream() {};

        size_t GetRemainingBitCount()  { return m_bit_capacity - m_bit_head; }
        size_t GetRemainingByteCount() { return ( GetRemainingBitCount() + 7 ) / 8; }
        size_t GetSize() { return ( 7 + m_bit_capacity ) / 8; }
        uint32_t SaveCurrentLocation() { return m_bit_head; }
        void SeekToLocation( uint32_t location ) { m_bit_head = location; }

        void Advance( uint32_t bit_cnt ) { m_bit_head += bit_cnt; }
        void ReadBits( void *out, uint32_t bit_cnt );
        void ReadBits( byte &out, uint32_t bit_cnt );
        template <typename T> void Read( T &data, uint32_t bit_cnt = sizeof(T) * 8 )
        {
            static_assert( std::is_arithmetic<T>::value
                        || std::is_enum<T>::value,
                           "Generic Read only supports primitive data types" );
            T read;
            ReadBits( &read, size * 8 );
            data = ByteSwap( read );
        }

        void Read( NetworkKey &out );
        void Read( sockaddr &out );

        void Read( uint64_t &out, uint32_t bit_cnt = 32 ) { uint32_t temp; ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Read( int64_t &out, uint32_t bit_cnt = 32 )  { int temp;      ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }

        void Read( uint32_t &out, uint32_t bit_cnt = 32 ) { uint32_t temp; ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Read( int& out, uint32_t bit_cnt = 32 )      { int temp;      ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
             
        void Read( uint16_t& out, uint32_t bit_cnt = 16 ) { uint16_t temp; ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Read( int16_t& out, uint32_t bit_cnt = 16 )  { int16_t temp;  ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
             
        void Read( bool& out )                            { bool temp;     ReadBits( &temp, 1 );       out = ByteSwap( temp ); }
        void Read( uint8_t& out, uint32_t bit_cnt = 8 )   { ReadBits( &out, bit_cnt ); }

        void Read( float& out )                           { float temp;    ReadBits( &temp, 32 );      out = ByteSwap( temp ); }

        void ReadBytes( void* out, size_t byte_cnt ) { ReadBits( out, byte_cnt * 8 ); }

    private:
        InputBitStream( byte *input, const size_t size, bool owned );
    };

    typedef std::shared_ptr<InputBitStream> InputBitStreamPtr;

    class InputBitStreamFactory
    {
    public:
        static InputBitStreamPtr CreateInputBitStream( byte *input, const size_t size, bool owned = true )
        {
            return InputBitStreamPtr( new InputBitStream( input, size, owned ) );
        }
    };

    class OutputBitStream : public BitStreamBase
    {
        friend class OutputBitStreamFactory;
    public:
        ~OutputBitStream() {};
    
        size_t GetCurrentBitCount()  { return m_bit_head; }
        size_t GetCurrentByteCount() { return ( GetCurrentBitCount() + 7 ) / 8; }
        size_t GetSize() { return GetCurrentByteCount(); }
        size_t Collapse();
    
        void WriteBits( void *out, uint32_t bit_cnt );
        void WriteBits( byte &out, uint32_t bit_cnt );

        template< typename T >
        void Write( T &data, uint32_t bit_cnt = sizeof(T) * 8 )
        {
            static_assert(std::is_arithmetic<T>::value
                       || std::is_enum<T>::value,
                          "Generic Write only supports primitive data types" );

            T write = ByteSwap( data );
            WriteBits( &write, bit_cnt );
        }
    
        void Write( NetworkKey &out );
        void Write( NetworkAuthentication &out );
        void Write( sockaddr &out );

        void Write( uint64_t out, uint32_t bit_cnt = 32 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write( int64_t out, uint32_t bit_cnt = 32 )  { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }

        void Write( uint32_t out, uint32_t bit_cnt = 32 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write( int out, uint32_t bit_cnt = 32 )      { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
             
        void Write( uint16_t out, uint32_t bit_cnt = 16 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write( int16_t out, uint32_t bit_cnt = 16 )  { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
             
        void Write( bool out )                            { auto temp = ByteSwap( out ); WriteBits( &temp, 1 );       }
        void Write( uint8_t out, uint32_t bit_cnt = 8 )   {                              WriteBits( &out, bit_cnt );  }

        void Write( float out )                           { auto temp = ByteSwap( out ); WriteBits( &temp, 32 );      }

        void WriteBytes( void* out, size_t byte_cnt ) { WriteBits( out, byte_cnt * 8 ); }

    private:
        OutputBitStream( byte *input, const size_t size, bool owned );
    };

    typedef std::shared_ptr<OutputBitStream> OutputBitStreamPtr;

    class OutputBitStreamFactory
    {
    public:
        static OutputBitStreamPtr CreateOutputBitStream( byte *input = nullptr, size_t size = 0, bool owned = true )
        {
            return OutputBitStreamPtr( new OutputBitStream( input, size, owned ) );
        }
    };

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

    union NetworkPacketPrefix
    {
        struct
        {
            byte packet_type       : NETWORK_PACKET_TYPE_BITS;
            byte sequence_byte_cnt : NETWORK_SEQUENCE_NUM_BITS;
        };

        uint8_t b;
    };

    typedef std::array<byte, NETWORK_CONNECT_TOKEN_RAW_LENGTH> NetworkConnectionTokenRaw;
    struct NetworkConnectionToken
    {
        uint64_t client_id;
        int32_t timeout_seconds;
        int32_t server_address_cnt;
        std::array<sockaddr, NETCODE_MAX_SERVERS_PER_CONNECT> server_addresses;
        NetworkKey client_to_server_key;
        NetworkKey server_to_client_key;
        NetworkAuthentication authentication;

        void Write( NetworkConnectionTokenRaw &raw );
        bool Read( NetworkConnectionTokenRaw &raw );
        static bool Decrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, uint64_t &expire_time, NetworkKey &key );
        static bool Encrypt( NetworkConnectionTokenRaw &raw, uint64_t sequence_num, uint64_t &protocol_id, uint64_t &expire_time, NetworkKey &key );
    };    
    typedef std::shared_ptr<NetworkConnectionToken> NetworkConnectionTokenPtr;

    struct NetworkConnectionRequestHeader
    {
        NetworkPacketPrefix prefix;
        std::array<char, NETWORK_PROTOCOL_VERSION_LEN> version;
        uint64_t protocol_id;
        uint64_t token_expiration_timestamp;
        uint64_t token_sequence;
        NetworkConnectionToken token;
    };
    
    typedef std::array<byte, NETWORK_CHALLENGE_TOKEN_RAW_LENGTH> NetworkChallengeTokenRaw;
    struct NetworkChallengeToken
    {
        uint64_t client_id;
        NetworkAuthentication authentication;

        void Write( NetworkChallengeTokenRaw &raw );
        bool Read( NetworkChallengeTokenRaw &raw );
        static bool Decrypt( NetworkChallengeTokenRaw &raw, uint64_t sequence_num, NetworkKey &key );
        static bool Encrypt( NetworkChallengeTokenRaw &raw, uint64_t sequence_num, NetworkKey &key );
    };

    struct NetworkPacketConnectionChallengeHeader
    {
        NetworkPacketPrefix prefix;
        uint64_t token_sequence;
        NetworkChallengeToken token;
    };

    struct NetworkPacketConnectionChallengeResponse
    {
        NetworkPacketPrefix prefix;
        uint64_t token_sequence;
        std::array<byte, NETWORK_CHALLENGE_TOKEN_RAW_LENGTH> challenge_token_data;
    };

    struct NetworkPacketKeepAlive
    {
        NetworkPacketPrefix prefix;
        //RtlGenRandom
    };

    struct NetworkPacketPayload
    {
        NetworkPacketPrefix prefix;
    };

    struct NetworkPacketDisconnect
    {
        NetworkPacketPrefix prefix;
    };

    struct NetworkPacketTypesAllowed
    {
        boolean allowed[ PACKET_TYPE_CNT ];

        inline void SetAllowed( const NetworkPacketType packet_type )   { allowed[ packet_type ] = true;  }
        inline boolean IsAllowed( const NetworkPacketType packet_type ) { return( allowed[packet_type] ); }

        NetworkPacketTypesAllowed() { ::ZeroMemory( allowed, sizeof(allowed) ); }
    };
    
    class NetworkPacket
    {
    public:
        NetworkPacketType packet_type;

        OutputBitStreamPtr WritePacket( uint64_t sequence_num, uint64_t protocol_id, NetworkKey &key );
    private:
        virtual void Write( OutputBitStreamPtr &out ) = 0;
    };

    class NetworkConnectionRequestPacket : public NetworkPacket
    {
    public:
        NetworkConnectionRequestHeader header;

        NetworkConnectionRequestPacket() { packet_type = PACKET_CONNECT_REQUEST; }

    private:
        virtual void Write( OutputBitStreamPtr &out );

    };

    class NetworkConnectionDeniedPacket : public NetworkPacket
    {
    public:
        NetworkConnectionDeniedPacket() { packet_type = PACKET_CONNECT_DENIED; }
    };

    class NetworkConnectionChallengePacket : public NetworkPacket
    {
    public:
        NetworkPacketConnectionChallengeHeader header;
        NetworkChallengeTokenRaw raw_token;

        NetworkConnectionChallengePacket() { packet_type = PACKET_CONNECT_CHALLENGE; }

    private:
        virtual void Write( OutputBitStreamPtr &out );

    };

    typedef std::shared_ptr<NetworkPacket> NetworkPacketPtr;

    class NetworkPacketFactory
    {
    public:
        static NetworkPacketPtr CreateIncomingConnectionRequest( NetworkConnectionRequestHeader &header );
        static NetworkPacketPtr CreateOutgoingConnectionDenied();
        static NetworkPacketPtr CreateOutgoingConnectionChallenge( NetworkPacketConnectionChallengeHeader &header, NetworkChallengeTokenRaw &token );
    };

    class NetworkCryptoMap
    {
    public:
        NetworkCryptoMap() {};
        
        NetworkAddressPtr address;
        uint64_t last_seen;
        uint64_t expire_time;
        int timeout_seconds;
        NetworkKey send_key;
        NetworkKey receive_key;

        bool IsExpired( uint64_t current_time )
        {
            if( timeout_seconds > 0
             && last_seen + timeout_seconds < current_time )
            {
            /* timed out */
            return false;
            }

            /* check expiration */
            return ( expire_time > 0.0 && expire_time < current_time );
        }

    };
    typedef std::shared_ptr<NetworkCryptoMap> NetworkCryptoMapPtr;

    class Networking
    {
        friend class NetworkingFactory;
    public:
        ~Networking();

        NetworkPacketPtr ReadPacket( uint64_t protocol_id, NetworkCryptoMapPtr &read_crypto, NetworkKey &private_key, NetworkPacketTypesAllowed &allowed, uint64_t now_time, InputBitStreamPtr &read );
        void SendPacket( Engine::NetworkSocketUDPPtr &socket, NetworkAddressPtr &to, NetworkPacketPtr &packet, uint64_t protocol_id, NetworkKey &key );

        uint32_t AddCryptoMap( NetworkAddressPtr &client_address, NetworkKey &send_key, NetworkKey &receive_key, uint64_t now_time, uint64_t expire_time, int timeout_secs );
        NetworkCryptoMapPtr FindCryptoMapByAddress( NetworkAddressPtr &search_address, uint64_t time );
        NetworkCryptoMapPtr FindCryptoMapByID( uint32_t search_id, NetworkAddressPtr &expected_address, uint64_t time );


        static bool Encrypt( void *data_to_encrypt, size_t data_length, byte* salt, size_t salt_length, NetworkNonce &nonce, NetworkKey &key );
        static bool Decrypt( void *data_to_decrypt, size_t data_length, byte *salt, size_t salt_length, NetworkNonce &nonce, NetworkKey &key );
        static void GenerateEncryptionKey( NetworkKey &key );

    private:
        WSADATA m_wsa_data;
        uint64_t m_sequence_num;
        std::map<uint32_t, NetworkCryptoMapPtr> m_crypto_map;
        uint32_t m_crypto_map_next_id;

        Networking();
        void Initialize();
    };

    typedef std::shared_ptr<Networking> NetworkingPtr;

    class NetworkingFactory
    {
    public:
        static NetworkingPtr StartNetworking();
    };
}
