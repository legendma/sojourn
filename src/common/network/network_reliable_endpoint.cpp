#include "pch.hpp"

#include "network_reliable_endpoint.hpp"
#include "network_message.hpp"

Engine::NetworkReliableEndpoint::NetworkReliableEndpoint() :
    next_message_sequence( 1 ),
    received_message_start_sequence( 0 ),
    round_trip_time( 0.2 )
{
}

bool Engine::NetworkReliableEndpoint::ProcessReceivedPackets( double now_time )
{
    while( in_queue.size() )
    {
        auto packet = in_queue.front();
        in_queue.pop();
        auto payload = reinterpret_cast<NetworkPayloadPacket&>( *packet );
        if( !received_packet_buffer.IsValidSequence( payload.header.sequence ) )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, L"NetworkReliableEndpoint::ProcessReceivedPackets ignored a packet with an out of date sequence." );
            continue;
        }

        auto &received_packet_info = received_packet_buffer.Insert( payload.header.sequence );
        received_packet_info.time_received = now_time;

        AckPackets( payload.header.packet_ack_recent_sequence, payload.header.packet_ack_sequence_bits, payload.header.start_message, now_time );
        if( payload.message_bytes 
         && !ReceiveMessages( payload.header.start_message, payload.header.message_data.data(), payload.message_bytes ) )
        {
            return false;
        }
    }

    QueueNewReceivedMessages();

    return true;
}

void Engine::NetworkReliableEndpoint::PackageOutgoingPackets( uint64_t client_id, double now_time )
{
    if( !out_messages.size() )
    {
        return;
    }

    NetworkPayloadHeader header;
    header.client_id = client_id;
    header.sequence = sent_packet_buffer.next_sequence;
    header.packet_ack_recent_sequence = received_packet_buffer.next_sequence - 1;
    header.packet_ack_sequence_bits = received_packet_buffer.GenerateAckBits();
    header.start_message = out_messages.front().sequence;

    auto write   = BitStreamFactory::CreateOutputBitStream( header.message_data.data(), header.message_data.size(), false );
    auto measure = BitStreamFactory::CreateMeasureBitStream();
    
    out_queue.clear();

    out_queue.emplace_back();
    out_queue.back().messages.cnt = 0;
    for( auto message : out_messages )
    {
        if( message.last_sent_time + NETWORK_MESSAGE_SEND_PERIOD / 1000.0 > now_time )
            continue;

        measure->Reset();
        measure->Write( message.sequence );
        message.message->Serialize( measure );

        if( write->GetCurrentByteCount() + measure->GetCurrentByteCount() > header.message_data.size()
         || out_queue.back().messages.cnt == out_queue.back().messages.sequences.size() )
        {
            out_queue.back().packet = Engine::NetworkPacketFactory::CreatePayload( header, write->GetCurrentByteCount() );
            sent_packet_buffer.next_sequence++;
            header.sequence = sent_packet_buffer.next_sequence;
            out_queue.emplace_back();
            out_queue.back().messages.cnt = 0;
            write->Reset();
        }

        write->Write( message.sequence - header.start_message );
        message.message->Serialize( write );
        out_queue.back().messages.sequences[ out_queue.back().messages.cnt++ ] = message.sequence;
    }

    if( out_queue.back().messages.cnt > 0 )
    {
        out_queue.back().packet = Engine::NetworkPacketFactory::CreatePayload( header, write->GetCurrentByteCount() );
        sent_packet_buffer.next_sequence++;
    }
    else
    {
        out_queue.pop_back();
    }
}

void Engine::NetworkReliableEndpoint::MarkSent( OutgoingPacket &packet, double now_time )
{
    auto payload = reinterpret_cast<NetworkPayloadPacket&>( *packet.packet );
    auto &info = sent_packet_buffer.Insert( payload.header.sequence );
    
    info.was_acked = false;
    info.time_sent = now_time;
    info.messages = packet.messages;
}

void Engine::NetworkReliableEndpoint::PushOutgoingMessage( Engine::NetworkMessagePtr message )
{
    QueuedMessage entry;
    entry.message = message;
    entry.sequence = next_message_sequence++;
    entry.last_sent_time = 0.0;

    out_messages.push_back( entry );
}

Engine::NetworkMessagePtr Engine::NetworkReliableEndpoint::PopIncomingMessage()
{
    if( !in_messages.size() )
    {
        return nullptr;
    }

    auto message = in_messages.front();
    in_messages.pop();

    return message;
}

void Engine::NetworkReliableEndpoint::AckPackets( uint16_t ack_sequence, uint32_t ack_bits, uint16_t start_message_sequence, double now_time )
{
    int flag = 1;
    for( auto i = 0; i < 32; i++ )
    {
        uint16_t sequence = ack_sequence - (uint16_t)i;
        if( ack_bits & flag )
        {
            auto &sent_packet_info = sent_packet_buffer.GetInfo( sequence );
            if( !sent_packet_info.was_acked )
            {
                RemoveAckedOutgoingMessages( start_message_sequence, sent_packet_info.messages );
            }

            sent_packet_info.was_acked = true;
            UpdateRTT( now_time - sent_packet_info.time_sent );
        }

        flag <<= 1;
    }
}

void Engine::NetworkReliableEndpoint::RemoveAckedOutgoingMessages( uint16_t start_sequence, MessageSequenceArray &messages )
{
    for( auto i = 0; i < messages.cnt; i++ )
    {
        uint16_t sequence = start_sequence + messages.sequences[ i ];
        auto it = std::find_if( out_messages.begin(), out_messages.end(), [sequence]( QueuedMessage const& msg )
        {
            return msg.sequence == sequence;
        } );

        if( it != out_messages.end() )
        {
            out_messages.erase( it );
        }
    }
    
}

bool Engine::NetworkReliableEndpoint::ReceiveMessages( uint16_t start_sequence, byte *message_data, size_t message_data_size )
{
    auto read = BitStreamFactory::CreateInputBitStream( message_data, message_data_size, false );
    while( read->GetRemainingByteCount() )
    {
        uint16_t sequence;
        read->Write( sequence );
        sequence += start_sequence;
                
        if( !received_message_buffer.Exists( sequence ) )
        {
            if( received_message_buffer.SequenceLessThan( sequence, received_message_start_sequence ) )
            {
                Engine::Log( Engine::LOG_LEVEL_ERROR, L"NetworkReliableEndpoint::ReceiveMessages message receive sequence buffer is corrupted!" );
                return false;
            }

            auto &info = received_message_buffer.Insert( sequence );
            info.message = Engine::NetworkMessageFactory::CreateMessage( read );
        }
    }

    if( received_message_buffer.SequenceGreaterThan( start_sequence, received_message_start_sequence ) )
    {
        received_message_start_sequence = start_sequence;
    }

    return true;
}

void Engine::NetworkReliableEndpoint::QueueNewReceivedMessages()
{
    auto start = received_message_start_sequence;
    for( auto i = 0; i < (int)received_message_buffer.entries.size(); i++ )
    {
        auto sequence = start + (uint16_t)i;
        if( !received_message_buffer.Exists( sequence ) )
        {
            break;
        }

        auto &info = received_message_buffer.GetInfo( sequence );
        assert( info.message );
        in_messages.push( info.message );
        info.message.reset();
        received_message_start_sequence++;
    }
}

void Engine::NetworkReliableEndpoint::UpdateRTT( double single_rtt )
{
    round_trip_time += NETWORK_RTT_SMOOTH_FACTOR * ( single_rtt - round_trip_time );
}
