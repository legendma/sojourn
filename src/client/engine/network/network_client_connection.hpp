#pragma once

#include "common/engine/engine_step_timer.hpp"
#include "common/network/network_main.hpp"
#include "common/network/network_matchmaking.hpp"
#include "engine/network/network_client_config.hpp"

namespace Engine
{
	class NetworkConnection
	{
        friend class NetworkConnectionFactory;
        friend class DisconnectedState;
        friend class SendingConnectRequestsState;
	public:
        typedef enum
        {
            DISCONNECTED,
            SENDING_CONNECT_REQUESTS,
            SENDING_CONNECT_REPLIES,
            CONNECTED
        } StateID;

        typedef enum
        {
            NO_CONNECTION_ERROR,
            NO_SERVER_GIVEN,
            SERVER_DNS_ERROR,
            SERVER_NOT_RESPONDING,
            SERVER_IS_FULL,
            SERVER_DISCONNECTED
        } ConnectionError;

        struct State
        {
            State( NetworkConnection &fsm ) : m_fsm( fsm ) {};
            virtual void EnterState() = 0;
            virtual void ExitState() = 0;
            virtual void Update() = 0;
            virtual StateID Id() = 0;

            NetworkConnection &m_fsm;
        }; typedef std::shared_ptr<State> StatePtr;

        void Update();
        void Connect( NetworkConnectionPassportPtr &offer );
        void TryNextServer();
        bool IsConnected() { return m_current_state->Id() == CONNECTED; }
        ConnectionError GetConnectionError();

    protected:
        std::map<StateID, StatePtr> m_states;
        StatePtr m_current_state;
        ConnectionError m_connect_error;
        NetworkSocketUDPPtr m_socket;
        NetworkAddressPtr m_server_address;
        NetworkAddressPtr m_our_address;
        NetworkConnectionPassportPtr m_passport;
        const NetworkClientConfig &m_config;
        StepTimer m_send_timer;
        NetworkConnectionTokenPtr m_token;
        
        NetworkConnection( const NetworkClientConfig &config );
        void ChangeState( StateID new_state );
	}; typedef std::shared_ptr<NetworkConnection> NetworkConnectionPtr;

    class NetworkConnectionFactory
    {
    public:
        static NetworkConnectionPtr CreateConnection( const NetworkClientConfig &config );
    };
}