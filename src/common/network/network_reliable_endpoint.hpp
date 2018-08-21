#pragma once

#include "network_main.hpp"
#include "network_message.hpp"

#define NETWORK_SEQUENCE_BUFFER_LENGTH     ( 1024 )
#define NETWORK_MAX_MESSAGES_PER_PACKET    ( 100 )

namespace Engine
{
    typedef std::array<uint32_t, NETWORK_MAX_MESSAGES_PER_PACKET> MessageIds;
    typedef struct
    {
        MessageIds ids;
        int cnt;
    } MessageIdBuffer;

    class NetworkReliableEndpoint
    {
    public:
        typedef struct
        {
            MessageIdBuffer messages;
            Engine::NetworkPacketPtr packet;
        } OutgoingPacket;

        NetworkReliableEndpoint();

        void ProcessReceivedPackets();
        void PackageOutgoing( double now_time );
        void MarkSent( OutgoingPacket& packet, uint16_t sequence_num, double now_time );
        void QueueMessage( NetworkMessagePtr message );

        std::queue<Engine::NetworkPacketPtr> in_queue;
        std::queue<OutgoingPacket> out_queue;

    private:
        uint16_t most_recent_received_sequence;
        uint32_t ack_sequence_bits;
        std::array<uint16_t, NETWORK_SEQUENCE_BUFFER_LENGTH> received_sequence_buffer;
        std::array<uint16_t, NETWORK_SEQUENCE_BUFFER_LENGTH> sent_sequence_buffer;

        typedef struct
        {
            bool was_acked;
            double time_sent;
            MessageIdBuffer messages;
        } PacketInfo;

        std::array<PacketInfo, NETWORK_SEQUENCE_BUFFER_LENGTH> sent_packet_info;

        typedef struct
        {
            uint32_t message_id;
            NetworkMessagePtr message;
        } QueuedMessage;

        uint32_t next_message_id;
        std::vector<QueuedMessage> out_messages;

    }; typedef std::shared_ptr<NetworkReliableEndpoint> NetworkReliableEndpointPtr;
}

