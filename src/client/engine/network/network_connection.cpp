#include "pch.hpp"
#include "network_connection.hpp"

#define NUMBER_OF_DISCONNECT_PACKETS ( 10 )

namespace Engine
{
class DisconnectedState : public Engine::NetworkConnection::State<Engine::NetworkConnection::DISCONNECTED>
{
public:
    DisconnectedState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State<Engine::NetworkConnection::DISCONNECTED>( fsm ){}

    virtual void ProcessPacket( Engine::NetworkPacketPtr &packet ){};

    virtual void EnterState()
    {
        m_fsm.m_allowed.Reset();
        m_fsm.m_server_address.reset();
        m_fsm.m_passport.reset();
    }

    virtual void ExitState()
    {
        m_fsm.m_connect_error = Engine::NetworkConnection::NO_CONNECTION_ERROR;
    }

    virtual void Update()
    {
        if( m_fsm.m_passport )
        {
             m_fsm.m_connect_start_time = Engine::Time::GetSystemTime();
             m_fsm.m_passport->current_server = -1;
             m_fsm.ChangeState( Engine::NetworkConnection::TRY_NEXT_SERVER );
        }
    }

};

class TryNextServerState : public Engine::NetworkConnection::State<Engine::NetworkConnection::TRY_NEXT_SERVER>
{
public:
    TryNextServerState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State<Engine::NetworkConnection::TRY_NEXT_SERVER>( fsm ){}

    virtual void ExitState(){};
    virtual void ProcessPacket( Engine::NetworkPacketPtr &packet ){};

    virtual void EnterState()
    {
        m_fsm.m_allowed.Reset();
        m_fsm.m_server_address.reset();
    }

    virtual void Update()
    {
        /* if the token has expired, stop trying to connect using it */
        if( m_fsm.IsPassportExpired() )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Connection token expired." );
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTED );
            return;
        }

        /* check if we have a server we haven't tried */
        m_fsm.m_passport->current_server++;
        if( m_fsm.NeedsMoreServers() )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, L"NetworkConnection::TryNextServerState ran out of servers while trying to connect." );
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTED );
            return;
        }

        /* Create the server address */
        m_fsm.m_server_address = Engine::NetworkAddressFactory::CreateFromAddress( m_fsm.m_passport->server_addresses[m_fsm.m_passport->current_server] );

        /* switch to request connection */
        Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::TryNextServer trying %s...", m_fsm.m_server_address->Print().c_str() );
        m_fsm.ChangeState( Engine::NetworkConnection::SENDING_CONNECT_REQUESTS );
    }

};

class SendingConnectRequestsState : public Engine::NetworkConnection::State<Engine::NetworkConnection::SENDING_CONNECT_REQUESTS>
{
    NetworkPacketPtr connect_request;

public:
    SendingConnectRequestsState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State<Engine::NetworkConnection::SENDING_CONNECT_REQUESTS>( fsm ){}

    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::SENDING_CONNECT_REQUESTS; }

    virtual void EnterState()
    {
        m_fsm.m_allowed.Reset();
        m_fsm.m_allowed.SetAllowed( PACKET_CONNECT_DENIED );
        m_fsm.m_allowed.SetAllowed( PACKET_CONNECT_CHALLENGE );
        m_fsm.m_connect_error = Engine::NetworkConnection::NO_CONNECTION_ERROR;

        m_fsm.m_last_recieved_packet_time = 0;

        NetworkConnectionRequestHeader request;
        request.version = m_fsm.m_passport->version;
        request.protocol_id = m_fsm.m_passport->protocol_id;
        request.token_expire_time = m_fsm.m_passport->token_expire_time;
        request.token_sequence = m_fsm.m_passport->token_sequence;
        request.raw_token = m_fsm.m_passport->raw_token;

        connect_request = Engine::NetworkPacketFactory::CreateConnectionRequest( &(*m_fsm.m_networking), request );

        m_fsm.ResetSendTimer();
    }

    virtual void ExitState()
    {
        connect_request.reset();
    }

    virtual void Update()
    {
        /* if the token has expired, stop trying to connect using it */
        if( m_fsm.IsPassportExpired() )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Connection token expired." );
            m_fsm.m_connect_error = Engine::NetworkConnection::SERVER_NOT_RESPONDING;
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTED );
            return;
        }

        if( !m_fsm.m_server_address )
        {
            Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::SendingConnectRequestsState unable to look server address." );
            m_fsm.m_connect_error = Engine::NetworkConnection::CANT_REACH_SERVER;
            m_fsm.ChangeState( Engine::NetworkConnection::TRY_NEXT_SERVER );
            return;
        }

        m_fsm.m_send_timer.Tick( [&]()
        {
            if( !m_fsm.m_networking->SendPacket( m_fsm.m_socket, m_fsm.m_server_address, connect_request, m_fsm.m_passport->protocol_id, m_fsm.m_passport->client_to_server_key, m_fsm.m_passport->token_sequence ) )
            {
                Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::SendingConnectRequestsState unable to send a connection request to %s...", m_fsm.m_server_address->Print().c_str() );
                m_fsm.m_connect_error = Engine::NetworkConnection::CANT_REACH_SERVER;
                m_fsm.ChangeState( Engine::NetworkConnection::TRY_NEXT_SERVER );
                return;
            }

            m_fsm.m_last_sent_packet_time = m_fsm.m_current_time;

        } );

    }

    virtual void ProcessPacket( Engine::NetworkPacketPtr &packet )
    {
        switch( packet->packet_type )
        {
            case PACKET_CONNECT_DENIED:
                Engine::Log( Engine::LOG_LEVEL_INFO, L"Connection to %s was denied.", m_fsm.m_server_address->Print().c_str() );
                m_fsm.m_connect_error = Engine::NetworkConnection::SERVER_IS_FULL;
                m_fsm.ChangeState( Engine::NetworkConnection::TRY_NEXT_SERVER );
                break;

            case PACKET_CONNECT_CHALLENGE:
                Engine::Log( Engine::LOG_LEVEL_INFO, L"Received connection challenge from %s.", m_fsm.m_server_address->Print().c_str() );
                auto challenge = reinterpret_cast<Engine::NetworkConnectionChallengePacket&>(*packet);
                m_fsm.m_challenge = challenge.header;
                m_fsm.m_last_recieved_packet_time = m_fsm.m_current_time;
                m_fsm.ChangeState( Engine::NetworkConnection::SENDING_CONNECT_REPLIES );
                break;
        }
    }
};

class SendingConnectRepliesState : public Engine::NetworkConnection::State<Engine::NetworkConnection::SENDING_CONNECT_REPLIES>
{
    NetworkPacketPtr connect_reply;

public:
    SendingConnectRepliesState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State<Engine::NetworkConnection::SENDING_CONNECT_REPLIES>( fsm ){}

    virtual void EnterState()
    {
        m_fsm.m_allowed.Reset();
        m_fsm.m_allowed.SetAllowed( PACKET_CONNECT_DENIED );
        m_fsm.m_allowed.SetAllowed( PACKET_KEEP_ALIVE );

        NetworkConnectionChallengeResponseHeader reply;
        reply.token_sequence = m_fsm.m_challenge.token_sequence;
        reply.raw_challenge_token = m_fsm.m_challenge.raw_challenge_token;

        connect_reply = Engine::NetworkPacketFactory::CreateConnectionChallengeResponse( &(*m_fsm.m_networking), reply );

        m_fsm.ResetSendTimer();
    }

    virtual void ExitState()
    {
        connect_reply.reset();
    }

    virtual void Update()
    {
        /* if the token has expired, stop trying to connect using it */
        if( m_fsm.IsPassportExpired() )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Connection token expired." );
            m_fsm.m_connect_error = Engine::NetworkConnection::SERVER_NOT_RESPONDING;
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTED );
            return;
        }

        m_fsm.m_send_timer.Tick( [&]()
        {
            if( !m_fsm.m_networking->SendPacket( m_fsm.m_socket, m_fsm.m_server_address, connect_reply, m_fsm.m_passport->protocol_id, m_fsm.m_passport->client_to_server_key, m_fsm.m_challenge.token_sequence ) )
            {
                Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::SendingConnectRepliesState unable to send a connection request to %s...", m_fsm.m_server_address->Print().c_str() );
                m_fsm.m_connect_error = Engine::NetworkConnection::CANT_REACH_SERVER;
                m_fsm.ChangeState( Engine::NetworkConnection::TRY_NEXT_SERVER );
                return;
            }

            m_fsm.m_last_sent_packet_time = m_fsm.m_current_time;

        } );
    }

    virtual void ProcessPacket( Engine::NetworkPacketPtr &packet )
    {
        switch( packet->packet_type )
        {
        case PACKET_CONNECT_DENIED:
            Engine::Log( Engine::LOG_LEVEL_INFO, L"Connection to %s was denied.", m_fsm.m_server_address->Print().c_str() );
            m_fsm.m_connect_error = Engine::NetworkConnection::SERVER_IS_FULL;
            m_fsm.ChangeState( Engine::NetworkConnection::TRY_NEXT_SERVER );
            break;

        case PACKET_KEEP_ALIVE:
            Engine::Log( Engine::LOG_LEVEL_INFO, L"Received connection accepted from %s.", m_fsm.m_server_address->Print().c_str() );
            m_fsm.m_keep_alive_packet = packet;
            m_fsm.m_last_recieved_packet_time = m_fsm.m_current_time;

            m_fsm.ChangeState( Engine::NetworkConnection::CONNECTED );
            break;
        }
    }
};

class ConnectedState : public Engine::NetworkConnection::State<Engine::NetworkConnection::CONNECTED>
{
public:
    ConnectedState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State<Engine::NetworkConnection::CONNECTED>( fsm ){}

    virtual void EnterState()
    {
        auto keep_alive = reinterpret_cast<Engine::NetworkKeepAlivePacket&>(*m_fsm.m_keep_alive_packet);
        m_fsm.m_client_id = keep_alive.header.client_id;
        m_fsm.m_send_packet_sequence = 0;

        m_fsm.m_allowed.Reset();
        m_fsm.m_allowed.SetAllowed( PACKET_KEEP_ALIVE );
        m_fsm.m_allowed.SetAllowed( PACKET_PAYLOAD );
        m_fsm.m_allowed.SetAllowed( PACKET_DISCONNECT );

        m_fsm.ResetSendTimer();

        m_fsm.m_endpoint = Engine::NetworkReliableEndpointPtr( new Engine::NetworkReliableEndpoint() );

        Engine::Log( Engine::LOG_LEVEL_INFO, L"Connected to %s!", m_fsm.m_server_address->Print().c_str() );
    }

    virtual void ExitState()
    {
        m_fsm.m_client_id = 0;
        m_fsm.m_keep_alive_packet.reset();
        m_fsm.m_endpoint.reset();
    }

    virtual void Update()
    {
        if( !m_fsm.m_keep_alive_packet )
        {
            m_fsm.m_connect_error = Engine::NetworkConnection::SYSTEM_ERROR;
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTING );
            return;
        }

        /* send keep alive */
        m_fsm.m_send_timer.Tick( [&]()
        {
            if( !m_fsm.m_networking->SendPacket( m_fsm.m_socket, m_fsm.m_server_address, m_fsm.m_keep_alive_packet, m_fsm.m_passport->protocol_id, m_fsm.m_passport->client_to_server_key, m_fsm.m_send_packet_sequence++ ) )
            {
                Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::ConnectedState unable to send a keep alive to %s...", m_fsm.m_server_address->Print().c_str() );
                return;
            }

            m_fsm.m_last_sent_packet_time = m_fsm.m_current_time;

        } );

        /* process packets received from server */
        if( !m_fsm.m_endpoint->ProcessReceivedPackets( m_fsm.m_current_time ) )
        {
            m_fsm.m_connect_error = Engine::NetworkConnection::SYSTEM_ERROR;
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTING );
            return;
        }

        /* send our packets to the server */
        m_fsm.m_endpoint->PackageOutgoingPackets( &(*m_fsm.m_networking), m_fsm.m_client_id, m_fsm.m_current_time );
        while( m_fsm.m_endpoint->out_queue.size() )
        {
            auto &outgoing = m_fsm.m_endpoint->out_queue.front();
            if( !m_fsm.m_networking->SendPacket( m_fsm.m_socket, m_fsm.m_server_address, outgoing.packet, m_fsm.m_passport->protocol_id, m_fsm.m_passport->client_to_server_key, m_fsm.m_send_packet_sequence++ ) )
            {
                Engine::Log( Engine::LOG_LEVEL_WARNING, L"NetworkConnection::ConnectedState failed to send payload packet to server." );
            }
            else
            {
                m_fsm.m_endpoint->MarkSent( outgoing, m_fsm.m_current_time );
            }

            m_fsm.m_endpoint->out_queue.pop_front();
        }

    }

    virtual void ProcessPacket( Engine::NetworkPacketPtr &packet )
    {
        switch( packet->packet_type )
        {
        case PACKET_KEEP_ALIVE:
            Engine::Log( Engine::LOG_LEVEL_INFO, L"Received connection accepted from %s.", m_fsm.m_server_address->Print().c_str() );
            m_fsm.m_last_recieved_packet_time = m_fsm.m_current_time;
            break;
        
        case PACKET_PAYLOAD:
            break;

        case PACKET_DISCONNECT:
            m_fsm.m_connect_error = Engine::NetworkConnection::SERVER_DISCONNECTED;
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTING );
            break;
        }

    }

};

class DisconnectingState : public Engine::NetworkConnection::State<Engine::NetworkConnection::DISCONNECTING>
{
    NetworkPacketPtr disconnect;
    int remaining_disconnects;

public:
    DisconnectingState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State<Engine::NetworkConnection::DISCONNECTING>( fsm ),
        remaining_disconnects( 0 ){}

    virtual void ProcessPacket( Engine::NetworkPacketPtr &packet ) {};

    virtual void EnterState()
    {
        m_fsm.m_allowed.Reset();

        disconnect = Engine::NetworkPacketFactory::CreateDisconnect( &(*m_fsm.m_networking) );
        remaining_disconnects = NUMBER_OF_DISCONNECT_PACKETS;
    }

    virtual void Update()
    {
        if( remaining_disconnects < 1 )
        {
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTED );
            return;
        }

        m_fsm.m_send_timer.Tick( [&]()
        {
            remaining_disconnects--;
            if( m_fsm.m_networking->SendPacket( m_fsm.m_socket, m_fsm.m_server_address, disconnect, m_fsm.m_passport->protocol_id, m_fsm.m_passport->client_to_server_key, m_fsm.m_passport->token_sequence ) )
            {
                Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::DisconnectingState unable to send a disconnect to %s...", m_fsm.m_server_address->Print().c_str() );
                m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTED );
                return;
            }

            m_fsm.m_last_sent_packet_time = m_fsm.m_current_time;

        } );
    }

    virtual void ExitState() 
    {
        disconnect.reset();
    }
};

}

Engine::NetworkConnection::NetworkConnection( const NetworkClientConfig &config, NetworkingPtr &networking ) :
    m_connect_error( NO_CONNECTION_ERROR ),
    m_config( config ),
    m_networking( networking ),
    m_client_id( 0 ),
    m_send_packet_sequence( 0 )
{
    /* create our address and the socket bound to it */
    m_our_address = Engine::NetworkAddressFactory::CreateAddressFromStringAsync( config.our_address ).get();
    if( !m_our_address )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"NetworkConnection could create client address." );
        throw std::runtime_error( "NetworkConnection" );
    }

    m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_our_address, config.receive_buff_size, config.send_buff_size );
    if( !m_socket )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"NetworkConnection could not create client socket." );
        throw std::runtime_error( "NetworkConnection" );
    }
            
    /* create the discrete states */
    auto disconnected = Engine::NetworkConnection::StatePtr( new DisconnectedState( *this ) );
    m_current_state = disconnected;
    m_states.insert( std::make_pair( DisconnectedState::id, disconnected ) );

    auto try_next_server = Engine::NetworkConnection::StatePtr( new TryNextServerState( *this ) );
    m_states.insert( std::make_pair( TryNextServerState::id, try_next_server ) );

    auto requests = Engine::NetworkConnection::StatePtr( new SendingConnectRequestsState( *this ) );
    m_states.insert( std::make_pair( SendingConnectRequestsState::id, requests ) );

    auto replies = Engine::NetworkConnection::StatePtr( new SendingConnectRepliesState( *this ) );
    m_states.insert( std::make_pair( SendingConnectRepliesState::id, replies ) );

    auto connected = Engine::NetworkConnection::StatePtr( new ConnectedState( *this ) );
    m_states.insert( std::make_pair( ConnectedState::id, connected ) );

    auto disconnecting = Engine::NetworkConnection::StatePtr( new DisconnectingState( *this ) );
    m_states.insert( std::make_pair( DisconnectingState::id, disconnecting ) );

    /* create the send timer */
    m_send_timer.SetFixedTimeStep( true );
    m_send_timer.SetTargetElapsedSeconds( m_config.connect_send_period / 1000.0f );

    m_current_time = Engine::Time::GetSystemTime();
}

void Engine::NetworkConnection::SendAndReceivePackets()
{
    m_current_time = Engine::Time::GetSystemTime();
    m_current_state->Update();
    ReceiveAndProcessPackets();
}

void Engine::NetworkConnection::ReceiveAndProcessPackets()
{
    byte data[NETWORK_MAX_PACKET_SIZE];
    while( true )
    {
        Engine::NetworkAddressPtr from;
        auto byte_cnt = m_socket->ReceiveFrom( data, sizeof( data ), from );
        if( byte_cnt == 0 )
            break;

        if( !m_passport
         || !m_server_address
         || !from->Matches( *m_server_address ) )
        {
            continue;
        }

        auto read = Engine::BitStreamFactory::CreateInputBitStream( data, byte_cnt, false );
        auto packet = Engine::NetworkPacket::ReadPacket( &(*m_networking), read, m_allowed, m_passport->protocol_id, m_passport->server_to_client_key, 0 );
        if( packet )
        {
            m_current_state->ProcessPacket( packet );
        }
        else
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, L"NetworkConnection::ReceiveAndProcessPackets received a packet it could not read or was not allowed." );
        }
    }
}

void Engine::NetworkConnection::Connect( Engine::NetworkConnectionPassportPtr &offer )
{
    /* if we are connecting already, then ignore the offer */
    if( m_current_state->GetId() != DISCONNECTED )
    {
        Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::Connect ignored passport offer.  Either connecting or connected." );
        return;
    }

    assert( offer );
    m_passport = offer;
}

bool Engine::NetworkConnection::NeedsMoreServers()
{
    if( !m_passport )
    {
        return true;
    }

    if( m_passport->current_server > m_passport->server_address_cnt - 1 )
    {
        return true;
    }

    return m_current_state->GetId() == DISCONNECTED;
}

Engine::NetworkMessagePtr Engine::NetworkConnection::PopIncomingMessage()
{
    if( !m_endpoint )
    {
        return nullptr;
    }

    return m_endpoint->PopIncomingMessage();
}

void Engine::NetworkConnection::ChangeState( StateID to_state )
{
    auto old_state = m_current_state;
    auto new_state = m_states[ to_state ];
    if( old_state == new_state )
    {
        return;
    }

    m_current_state = new_state;
    old_state->ExitState();
    m_current_state->EnterState();
}

bool Engine::NetworkConnection::IsPassportExpired()
{
    if( !m_passport )
    {
        return true;
    }

    auto token_expiry = m_passport->token_expire_time - m_passport->token_create_time;
    auto attempting_duration = m_current_time - m_connect_start_time;

    return attempting_duration > token_expiry;
}

Engine::NetworkConnectionPtr Engine::NetworkConnectionFactory::CreateConnection( const NetworkClientConfig &config, NetworkingPtr &networking )
{
    try
    {
        return Engine::NetworkConnectionPtr( new NetworkConnection( config, networking ) );
    }
    catch( std::runtime_error() ) {}

    return nullptr;
}
