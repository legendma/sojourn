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
        m_fsm.m_token.reset();
    }

    virtual void ExitState()
    {
        m_fsm.m_connect_error = Engine::NetworkConnection::NO_CONNECTION_ERROR;
    }

    virtual void Update()
    {
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
        m_fsm.m_token = Engine::NetworkConnectionTokenPtr( new NetworkConnectionToken() );
    }

    virtual void ExitState()
    {

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
    }

    virtual void ExitState()
    {

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
    m_connect_error( NO_SERVER_GIVEN ),
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

    // Create the connection socket
    m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_server_address, m_config.send_buff_size, m_config.receive_buff_size );
    if( m_socket == nullptr )
    {
        Engine::ReportError( L"NetworkConnection::NetworkConnection Unable to create client socket." );
        throw std::runtime_error( "CreateUDPSocket" );
    }
}

void Engine::NetworkConnection::Update()
{
    m_current_state->Update();
}

bool Engine::NetworkConnection::ConnectTo( std::wstring &server_address )
{
    // Create the server address
    m_server_address = Engine::NetworkAddressFactory::CreateAddressFromStringAsync( server_address ).get();
    if( m_server_address == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Connection request failed.  Could not lookup server address." );
        m_connect_error = SERVER_DNS_ERROR;
        return false;
    }

    /* switch to request connection */
    Engine::Log( Engine::LOG_LEVEL_INFO, L"Connecting to %s...", m_server_address->Print().c_str() );
    ChangeState( SENDING_CONNECT_REQUESTS );

    return true;
}

Engine::NetworkConnection::ConnectionError Engine::NetworkConnection::GetConnectionError()
{
    return Engine::NetworkConnection::ConnectionError();
}

void Engine::NetworkConnection::ChangeState( StateID new_state )
{
    auto old_state = m_current_state;
    m_current_state = m_states[ new_state ];
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
