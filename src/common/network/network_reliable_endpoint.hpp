#pragma once

#include "network_main.hpp"
#include "network_message.hpp"

#define NETWORK_SEQUENCE_BUFFER_LENGTH     ( 1024 )
#define NETWORK_MAX_MESSAGES_PER_PACKET    ( 100 )
#define NETWORK_MESSAGE_SEND_PERIOD        ( 100 )
#define NETWORK_RTT_SMOOTH_FACTOR          ( 0.0025f )

namespace Engine
{
    template <typename PacketInfoType, size_t Size>
    struct SequenceBuffer
    {
        uint16_t next_sequence;
        std::array<uint32_t, Size> entries;
        std::array<PacketInfoType, Size> info;

        SequenceBuffer() : 
            next_sequence( 0 )
        { 
            std::memset( &entries, 0xffffffff, sizeof( entries ) );
        }

        inline bool SequenceLessThan( uint16_t a, uint16_t b )
        {
            return ( a < b && b - a <  0x00ff )
                || ( b < a && a - b >= 0x00ff );
        }

        inline bool SequenceGreaterThan( uint16_t a, uint16_t b )
        {
            return SequenceLessThan( b, a );
        }

        inline void Remove( uint16_t sequence )
        {
            entries[ sequence % entries.size() ] = 0xffffffff;
        }

        void Remove( int from, int to )
        {
            if( to < from )
            {
                to += 0xffff;
            }

            assert( to - from < (int)entries.size() );
            for( auto i = from; i <= to; i++ )
            {
                Remove( i );
            }
        }

        inline bool IsValidSequence( uint16_t test )
        {
            return SequenceLessThan( test, next_sequence - (uint16_t)entries.size() );
        }

        PacketInfoType & Insert( uint16_t sequence )
        {
            assert( IsValidSequence( sequence ) );
            /* if this sequence is higher than our previous highest sequence, clear any
               gaps (potential lost packets) to invalid, then update previous highest   */
            if( SequenceGreaterThan( sequence, next_sequence - 1 ) )
            {
                Remove( next_sequence, sequence );
                next_sequence = sequence + 1;
            }

            auto index = sequence % entries.size();
            entries[ index ] = sequence;
            return info[ index ];
        }

        inline bool Exists( uint16_t sequence )
        {
            return entries[ sequence % entries.size() ] == sequence;
        }

        uint32_t GenerateAckBits()
        {
            uint16_t last_highest_sequence = next_sequence - 1;
            uint32_t ack_bits = 0;
            uint32_t flag = 1;
            for( auto i = 0; i < 32; i++ )
            {
                uint16_t sequence = last_highest_sequence - (uint16_t)i;
                if( Exists( sequence ) )
                {
                    ack_bits |= flag;
                }

                flag <<= 1;
            }

            return ack_bits;
        }

        PacketInfoType & GetInfo( uint16_t sequence )
        {
            auto index = sequence % entries.size();
            assert( entries[ index ] == sequence );
            return info[ index ];
        }

    };

    typedef std::array<uint16_t, NETWORK_MAX_MESSAGES_PER_PACKET> MessageSequences;
    typedef struct
    {
        MessageSequences sequences;
        int cnt;
    } MessageSequenceArray;

    class NetworkReliableEndpoint
    {
    public:
        typedef struct
        {
            MessageSequenceArray messages;
            Engine::NetworkPacketPtr packet;
        } OutgoingPacket;

        NetworkReliableEndpoint();

        bool ProcessReceivedPackets( double now_time );
        void PackageOutgoingPackets( IMemoryAllocator *allocator, uint64_t client_id, double now_time );
        void MarkSent( OutgoingPacket &packet, double now_time );
        void PushOutgoingMessage( NetworkMessagePtr message );
        NetworkMessagePtr PopIncomingMessage();

        std::deque<OutgoingPacket> out_queue;
        std::queue<Engine::NetworkPacketPtr> in_queue;
        double round_trip_time;

    private:
        /* packet send */
        typedef struct
        {
            bool was_acked;
            double time_sent;
            MessageSequenceArray messages;
        } SentPacketInfo;

        SequenceBuffer<SentPacketInfo, NETWORK_SEQUENCE_BUFFER_LENGTH> sent_packet_buffer;

        /* packet receive */
        typedef struct
        {
            double time_received;
        } ReceivedPacketInfo;

        SequenceBuffer<ReceivedPacketInfo, NETWORK_SEQUENCE_BUFFER_LENGTH> received_packet_buffer;

        /* message send */
        typedef struct
        {
            NetworkMessagePtr message;
            double last_sent_time;
            uint16_t sequence;
        } QueuedMessage;

        uint32_t next_message_sequence;
        std::vector<QueuedMessage> out_messages;

        /* message receive */
        typedef struct
        {
            NetworkMessagePtr message;
        } ReceivedMessageInfo;

        SequenceBuffer<ReceivedMessageInfo, NETWORK_SEQUENCE_BUFFER_LENGTH> received_message_buffer;
        uint16_t received_message_start_sequence;
        std::queue<NetworkMessagePtr> in_messages;

        void AckPackets( uint16_t ack_sequence, uint32_t ack_bits, uint16_t start_message_sequence, double now_time );
        void RemoveAckedOutgoingMessages( uint16_t start_sequence, MessageSequenceArray &messages );
        bool ReceiveMessages( uint16_t start_sequence, byte *message_data, size_t message_data_size );
        void QueueNewReceivedMessages();
        void UpdateRTT( double single_rtt );

    }; typedef std::shared_ptr<NetworkReliableEndpoint> NetworkReliableEndpointPtr;
}

