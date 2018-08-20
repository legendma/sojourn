#include "pch.hpp"

#include "network_reliable_endpoint.hpp"

void Engine::NetworkReliableEndpoint::ProcessReceivedPackets()
{
}

//void Engine::NetworkReliableEndpoint::ProcessReceivedMessages()
//{
//}

void Engine::NetworkReliableEndpoint::PackageOutgoing()
{
}

void Engine::NetworkReliableEndpoint::MarkSent( Engine::NetworkPayloadPacket &packet, uint16_t sequence_num, double now_time )
{
    int index = sequence_num % sent_sequence_buffer.size();
    sent_sequence_buffer[ index ] = sequence_num;

    auto &info = sent_packet_info[ index ];
    info.was_acked = false;
    info.time_sent = now_time;
}
