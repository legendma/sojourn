#pragma once

#include "network_main.hpp"

#define NETWORK_SEQUENCE_BUFFER_LENGTH     ( 1024 )

namespace Engine
{
    struct NetworkReliableEndpoint
    {
        typedef struct
        {
            bool was_acked;
            double time_sent;
            // TODO <MPA>: Add array of message IDs that were in this packet
        } PacketInfo;

        void ProcessReceivedPackets();
        //void ProcessReceivedMessages();
        void PackageOutgoing();
        void MarkSent( Engine::NetworkPayloadPacket& packet, uint16_t sequence_num, double now_time );

        std::queue<Engine::NetworkPacketPtr> in_queue;
        std::queue<Engine::NetworkPacketPtr> out_queue;

    private:
        uint16_t most_recent_received_sequence;
        uint32_t ack_sequence_bits;
        std::array<uint16_t, NETWORK_SEQUENCE_BUFFER_LENGTH> received_sequence_buffer;
        std::array<uint16_t, NETWORK_SEQUENCE_BUFFER_LENGTH> sent_sequence_buffer;
        std::array<PacketInfo, NETWORK_SEQUENCE_BUFFER_LENGTH> sent_packet_info;

    }; typedef std::shared_ptr<NetworkReliableEndpoint> NetworkReliableEndpointPtr;
}

