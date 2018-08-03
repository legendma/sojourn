#include "pch.hpp"
#include "network_client_connection.hpp"

namespace Engine
{
class DisconnectedState : public Engine::NetworkConnection::State
{
public:
    DisconnectedState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State( fsm )
    {
    }

    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::DISCONNECTED; }

    virtual void EnterState()
    {
        m_fsm.m_server_address.reset();
        m_fsm.m_allowed.Reset();
        m_fsm.m_socket.reset();
        m_fsm.m_passport.reset();
    }

    virtual void ExitState()
    {
        m_fsm.m_connect_error = Engine::NetworkConnection::NO_CONNECTION_ERROR;
    }

    virtual void Update()
    {
    }
};

class TryNextServerState : public Engine::NetworkConnection::State
{
public:
    TryNextServerState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State( fsm )
    {
    }

    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::TRY_NEXT_SERVER; }

    virtual void EnterState()
    {
    }

    virtual void ExitState()
    {
    }

    virtual void Update()
    {
        /* check if we have a server we haven't tried */
        if( m_fsm.NeedsMoreServers() )
        {
            Engine::Log( Engine::LOG_LEVEL_DEBUG, L"NetworkConnection::TryNextServerState ran out of servers while trying to connect." );
            m_fsm.ChangeState( Engine::NetworkConnection::DISCONNECTED );
            return;
        }

        /* Create the server socket */
        m_fsm.m_server_address = Engine::NetworkAddressFactory::CreateFromAddress( m_fsm.m_passport->server_addresses[++m_fsm.m_passport->current_server] );
        m_fsm.m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_fsm.m_server_address, m_fsm.m_config.send_buff_size, m_fsm.m_config.receive_buff_size );
        if( m_fsm.m_socket == nullptr )
        {
            m_fsm.m_server_address.reset();
            Engine::Log( Engine::LOG_LEVEL_WARNING, L"NetworkConnection::TryNextServerState socket creation failed." );
            m_fsm.m_connect_error = Engine::NetworkConnection::SERVER_SOCKET_ERROR;
            return;
        }

        /* switch to request connection */
        Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::TryNextServer trying %s...", m_fsm.m_server_address->Print().c_str() );
        m_fsm.ChangeState( Engine::NetworkConnection::SENDING_CONNECT_REQUESTS );
    }
};

class SendingConnectRequestsState : public Engine::NetworkConnection::State
{
public:
    SendingConnectRequestsState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State( fsm )
    {
    }

    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::SENDING_CONNECT_REQUESTS; }

    virtual void EnterState()
    {
        m_fsm.m_allowed.SetAllowed( PACKET_CONNECT_DENIED );
        m_fsm.m_allowed.SetAllowed( PACKET_CONNECT_CHALLENGE );
        m_fsm.m_connect_error = Engine::NetworkConnection::NO_CONNECTION_ERROR;
    }

    virtual void ExitState()
    {
        m_fsm.m_allowed.Reset();
    }

    virtual void Update()
    {
        m_fsm.m_send_timer.Tick( [&]()
        {

        } );

    }
};

class SendingConnectRepliesState : public Engine::NetworkConnection::State
{
public:
    SendingConnectRepliesState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State( fsm )
    {
    }

    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::SENDING_CONNECT_REPLIES; }

    virtual void EnterState()
    {
        m_fsm.m_allowed.SetAllowed( PACKET_CONNECT_DENIED );
        m_fsm.m_allowed.SetAllowed( PACKET_KEEP_ALIVE );
    }

    virtual void ExitState()
    {
        m_fsm.m_allowed.Reset();
    }

    virtual void Update()
    {

    }
};

class ConnectedState : public Engine::NetworkConnection::State
{
public:
    ConnectedState( Engine::NetworkConnection &fsm ) :
        Engine::NetworkConnection::State( fsm )
    {
    }
    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::CONNECTED; }

    virtual void EnterState()
    {
        m_fsm.m_allowed.SetAllowed( PACKET_KEEP_ALIVE );
        m_fsm.m_allowed.SetAllowed( PACKET_PAYLOAD );
        m_fsm.m_allowed.SetAllowed( PACKET_DISCONNECT );
    }

    virtual void ExitState()
    {
        
    }

    virtual void Update()
    {

    }
};

}

Engine::NetworkConnection::NetworkConnection( const NetworkClientConfig &config ) :
    m_connect_error( NO_CONNECTION_ERROR ),
    m_config( config )
{
    /* create the discrete states */
    auto disconnected = Engine::NetworkConnection::StatePtr( new DisconnectedState( *this ) );
    m_current_state = disconnected;
    m_states.insert( std::make_pair( disconnected->Id(), disconnected ) );

    auto requests = Engine::NetworkConnection::StatePtr( new SendingConnectRequestsState( *this ) );
    m_states.insert( std::make_pair( requests->Id(), requests ) );

    auto replies = Engine::NetworkConnection::StatePtr( new SendingConnectRepliesState( *this ) );
    m_states.insert( std::make_pair( replies->Id(), replies ) );

    auto connected = Engine::NetworkConnection::StatePtr( new ConnectedState( *this ) );
    m_states.insert( std::make_pair( connected->Id(), connected ) );

    /* create the send timer */
    m_send_timer.SetFixedTimeStep( true );
    m_send_timer.SetTargetElapsedSeconds( m_config.connect_send_period / 1000.0f );
}

void Engine::NetworkConnection::Update()
{
    m_current_state->Update();
}

void Engine::NetworkConnection::Connect( Engine::NetworkConnectionPassportPtr &offer )
{
    /* if we are connecting already, then ignore the offer */
    if( m_current_state->Id() != DISCONNECTED )
    {
        Engine::Log( Engine::LOG_LEVEL_INFO, L"NetworkConnection::Connect ignored passport offer.  Either connecting or connected." );
        return;
    }

    assert( offer );
    m_passport = offer;
    m_passport->current_server = -1;
    ChangeState( TRY_NEXT_SERVER );
}

bool Engine::NetworkConnection::NeedsMoreServers()
{
    if( !m_passport )
    {
        return true;
    }

    if( m_current_state->Id() != DISCONNECTED )
    {
        return false;
    }

    auto has_more_servers = m_passport->current_server < m_passport->server_address_cnt - 1;
    return !has_more_servers;
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

Engine::NetworkConnectionPtr Engine::NetworkConnectionFactory::CreateConnection( const NetworkClientConfig &config )
{
    try
    {
        return Engine::NetworkConnectionPtr( new NetworkConnection( config ) );
    }
    catch( std::runtime_error() ) {}

    return( nullptr );
}
