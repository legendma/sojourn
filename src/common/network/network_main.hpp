#pragma once

#include "common/network/network_platform.hpp"
#include "common/network/network_sockets.hpp"
#include "common/network/network_address.hpp"

#define NETWORK_ENDIANNESS               LITTLE_ENDIAN

#define NETWORK_PRIVATE_KEY_BYTE_CNT     ( 32 )
#define NETWORK_SOJOURN_PROTOCOL_ID      ( 0x1ceb00dacafebabe )
#define NETWORK_MAX_PACKET_SIZE          ( 1200 )
#define NETWORK_PROTOCOL_VERSION_LEN     ( 16 )
#define NETWORK_PROTOCOL_VERSION         ( "Sojourn Ver 1.0\0" )
#define NETWORK_CONNECT_TOKEN_BYTES      ( 1024 )
#define NETCODE_MAX_SERVERS_PER_CONNECT  ( 32 )
#define NETWORK_CHALLENGE_TOKEN_BYTES    ( 250 )

#define NETWORK_PACKET_TYPE_BITS         ( 4 )
#define NETWORK_SEQUENCE_NUM_BITS        ( 4 )

#define NETWORK_KEY_LENGTH               ( crypto_aead_chacha20poly1305_IETF_KEYBYTES )
#define NETWORK_AUTHENTICATION_LENGTH    ( crypto_aead_chacha20poly1305_IETF_ABYTES )

namespace Engine
{
    typedef std::array<byte, NETWORK_KEY_LENGTH> NetworkKey;
    typedef std::array<byte, NETWORK_AUTHENTICATION_LENGTH> NetworkAuthentication;

    class BitStreamBase
    {
    public:
        BitStreamBase( bool is_owned );
        ~BitStreamBase() { std::free( m_buffer ); }

        size_t GetSize() { return m_bit_capacity * 8; }
        byte * GetBuffer() { return m_buffer; }
        static int BitsRequired( uint64_t value );
        
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

        void Read( uint32_t &out, uint32_t bit_cnt = 32 ) { uint32_t temp; ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Read( int& out, uint32_t bit_cnt = 32 )      { int temp;      ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Read( float& out )                           { float temp;    ReadBits( &temp, 32 );      out = ByteSwap( temp ); }
             
        void Read( uint16_t& out, uint32_t bit_cnt = 16 ) { uint16_t temp; ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
        void Read( int16_t& out, uint32_t bit_cnt = 16 )  { int16_t temp;  ReadBits( &temp, bit_cnt ); out = ByteSwap( temp ); }
             
        void Read( bool& out )                            { bool temp;     ReadBits( &temp, 1 );       out = ByteSwap( temp ); }
        void Read( uint8_t& out, uint32_t bit_cnt = 8 )   { ReadBits( &out, bit_cnt ); }

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
    
        size_t GetCurrentBitCount()  { return m_bit_capacity - m_bit_head; }
        size_t GetCurrentByteCount() { return ( GetCurrentBitCount() + 7 ) / 8; }
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
        void Write( sockaddr &out );

        void Write( uint32_t &out, uint32_t bit_cnt = 32 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write( int& out, uint32_t bit_cnt = 32 )      { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write( float& out )                           { auto temp = ByteSwap( out ); WriteBits( &temp, 32 );      }
             
        void Write( uint16_t& out, uint32_t bit_cnt = 16 ) { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
        void Write( int16_t& out, uint32_t bit_cnt = 16 )  { auto temp = ByteSwap( out ); WriteBits( &temp, bit_cnt ); }
             
        void Write( bool& out )                            { auto temp = ByteSwap( out ); WriteBits( &temp, 1 );       }
        void Write( uint8_t& out, uint32_t bit_cnt = 8 )   {                              WriteBits( &out, bit_cnt );  }

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
    

    struct NetworkConnectionRequestHeader
    {
        NetworkPacketPrefix prefix;
        std::array<char, NETWORK_PROTOCOL_VERSION_LEN> version;
        uint64_t protocol_id;
        uint64_t connection_token_expiration_timestamp;
        uint64_t connection_token_sequence;
        std::array<byte, NETWORK_CONNECT_TOKEN_BYTES> connection_token_data;
    };

    struct NetworkConnectionToken
    {
        uint64_t client_id;
        int32_t timeout_seconds;
        int32_t server_address_cnt;
        std::array<sockaddr, NETCODE_MAX_SERVERS_PER_CONNECT> server_addresses;
        NetworkKey client_to_server_key;
        NetworkKey server_to_client_key;
        NetworkAuthentication token_uid;
    };

    typedef std::shared_ptr<NetworkConnectionToken> NetworkConnectionTokenPtr;

    struct NetworkPacketConnectionDenied
    {
        NetworkPacketPrefix prefix;
    };

    struct NetworkPacketConnectionChallenge
    {
        NetworkPacketPrefix prefix;
        uint64_t challenge_token_sequence;
        std::array<byte, NETWORK_CHALLENGE_TOKEN_BYTES> challenge_token_data;
    };

    struct NetworkPacketConnectionChallengeResponse
    {
        NetworkPacketPrefix prefix;
        uint64_t challenge_token_sequence;
        std::array<byte, NETWORK_CHALLENGE_TOKEN_BYTES> challenge_token_data;
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

        OutputBitStreamPtr Get( uint64_t sequence_num );
    private:
        virtual void Write( OutputBitStreamPtr &out ) = 0;
    };

    class NetworkConnectionRequestPacket : public NetworkPacket
    {
    public:
        NetworkConnectionRequestHeader header;
        NetworkAddressPtr from_address;

        NetworkConnectionTokenPtr ReadToken();
        NetworkConnectionRequestPacket() { packet_type = PACKET_CONNECT_REQUEST; }

    private:
        virtual void Write( OutputBitStreamPtr &out );

    };

    class NetworkConnectionDeniedPacket : public NetworkPacket
    {
    public:
        NetworkConnectionDeniedPacket() { packet_type = PACKET_CONNECT_DENIED; }
    };

    typedef std::shared_ptr<NetworkPacket> NetworkPacketPtr;

    class NetworkPacketFactory
    {
    public:
        static NetworkPacketPtr CreateConnectionRequest( NetworkConnectionRequestHeader &header, NetworkAddressPtr &from );
        static NetworkPacketPtr CreateConnectionDenied();
    };

    interface IProcessesPackets
    {
        virtual void OnReceivedConnectionRequest( NetworkPacketPtr &packet ) = 0;
    };

    class Networking
    {
        friend class NetworkingFactory;
    public:
        ~Networking();

        void ReadAndProcessPacket( uint64_t protocol_id, NetworkPacketTypesAllowed &allowed, NetworkAddressPtr &from, uint64_t now_time, InputBitStreamPtr &read, IProcessesPackets &processor );
        void ProcessPacket( IProcessesPackets &process, NetworkPacketPtr &packet );
        void SendPacket( Engine::NetworkSocketUDPPtr &socket, NetworkAddressPtr &to, NetworkPacketPtr &packet );

        static bool Encrypt( byte *data_to_encrypt, size_t data_length, byte* salt, size_t salt_length, byte *nonce, NetworkKey key );

    private:
        WSADATA m_wsa_data;
        uint64_t m_sequence_num;

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

