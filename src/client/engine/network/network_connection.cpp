#include "pch.hpp"
#include "network_connection.hpp"

class DisconnectedState : public Engine::NetworkConnection::State
{
public:
    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::DISCONNECTED; }

    virtual void EnterState()
    {
    }

    virtual void ExitState()
    {

    }
};

class SendingConnectRequestsState : public Engine::NetworkConnection::State
{
public:
    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::SENDING_CONNECT_REQUESTS; }

    virtual void EnterState()
    {
    }

    virtual void ExitState()
    {

    }
};

class SendingConnectRepliesState : public Engine::NetworkConnection::State
{
public:
    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::SENDING_CONNECT_REPLIES; }

    virtual void EnterState()
    {
    }

    virtual void ExitState()
    {

    }
};

class ConnectedState : public Engine::NetworkConnection::State
{
public:
    virtual Engine::NetworkConnection::StateID Id() { return Engine::NetworkConnection::CONNECTED; }

    virtual void EnterState()
    {
    }

    virtual void ExitState()
    {

    }
};

Engine::NetworkConnection::NetworkConnection()
{
    auto disconnected = Engine::NetworkConnection::StatePtr( new DisconnectedState() );
    m_current_state = disconnected;
    m_states.insert( std::make_pair( disconnected->Id(), disconnected ) );

    auto requests = Engine::NetworkConnection::StatePtr( new SendingConnectRequestsState() );
    m_states.insert( std::make_pair( requests->Id(), requests ) );

    auto replies = Engine::NetworkConnection::StatePtr( new SendingConnectRepliesState() );
    m_states.insert( std::make_pair( replies->Id(), replies ) );

    auto connected = Engine::NetworkConnection::StatePtr( new ConnectedState() );
    m_states.insert( std::make_pair( connected->Id(), connected ) );
}

void Engine::NetworkConnection::Update()
{
}

bool Engine::NetworkConnection::ConnectTo( std::wstring server_address )
{
    return false;
}

void Engine::NetworkConnection::ChangeState( StateID new_state )
{
    auto old_state = m_current_state;
    m_current_state = m_states[ new_state ];
    old_state->ExitState();
    m_current_state->EnterState();
}
