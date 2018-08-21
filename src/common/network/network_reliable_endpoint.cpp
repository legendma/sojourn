#include "pch.hpp"

#include "network_reliable_endpoint.hpp"

#include "network_message.hpp"

Engine::NetworkReliableEndpoint::NetworkReliableEndpoint() :
    most_recent_received_sequence( 0 ),
    ack_sequence_bits( 0 ),
    next_message_id( 1 )
{
}

void Engine::NetworkReliableEndpoint::ProcessReceivedPackets()
{

}

void Engine::NetworkReliableEndpoint::PackageOutgoing( double now_time )
{
    auto msg = Engine::NetworkMessageFactory::CreateMessage( MESSAGE_TEST );
    auto measure = Engine::BitStreamFactory::CreateMeasureBitStream();
    msg->Serialize( measure );

    auto read = Engine::BitStreamFactory::CreateInputBitStream( nullptr, 0 );
    msg->Serialize( read );

    auto write = Engine::BitStreamFactory::CreateOutputBitStream();
    msg->Serialize( write );
}

void Engine::NetworkReliableEndpoint::MarkSent( OutgoingPacket &packet, uint16_t sequence_num, double now_time )
{
    int index = sequence_num % sent_sequence_buffer.size();
    sent_sequence_buffer[ index ] = sequence_num;

    auto &info = sent_packet_info[ index ];
    info.was_acked = false;
    info.time_sent = now_time;
    info.messages = packet.messages;
}

void Engine::NetworkReliableEndpoint::QueueMessage( Engine::NetworkMessagePtr message )
{
    QueuedMessage entry;
    entry.message = message;
    entry.message_id = next_message_id++;

    out_messages.push_back( entry );
}
