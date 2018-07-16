#pragma once

namespace Engine
{
    typedef enum
    {
        PACKET_REQUEST,
        PACKET_DENIED,
        PACKET_CHALLENGE,
        PACKET_RESPONSE,
        PACKET_KEEP_ALIVE,
        PACKET_PAYLOAD,
        PACKET_DISCONNECT,
        PACKET_TYPE_CNT
    } PacketType;

    struct PacketTypesAllowed
    {
        boolean allowed[ PACKET_TYPE_CNT ];

        inline void SetAllowed( const PacketType packet_type ) { allowed[ packet_type ] = true; }
        inline boolean IsAllowed( const PacketType packet_type ) { return( allowed[packet_type] ); }

        PacketTypesAllowed() { ::ZeroMemory( allowed, sizeof(allowed) ); }
    };

    class Networking
    {
        friend class NetworkingFactory;
    public:
        ~Networking();

    private:
        WSADATA m_wsa_data;

        Networking();
        void Initialize();
    };

    typedef std::shared_ptr<Networking> NetworkingPtr;

    class NetworkingFactory
    {
    public:
        static NetworkingPtr CreateNetworking();
    };
}

