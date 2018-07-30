#pragma once

namespace Engine
{
	class NetworkConnection
	{
	public:
        typedef enum
        {
            DISCONNECTED,
            SENDING_CONNECT_REQUESTS,
            SENDING_CONNECT_REPLIES,
            CONNECTED

        } StateID;

        class State
        {
        public:
            virtual void EnterState() = 0;
            virtual void ExitState() = 0;
            virtual StateID Id() = 0;
        };
        typedef std::shared_ptr<State> StatePtr;

        NetworkConnection();
        void Update();
        bool ConnectTo( std::wstring server_address );
        bool IsConnected() { return m_current_state->Id() == CONNECTED; }

    protected:
        std::map<StateID, StatePtr> m_states;
        StatePtr m_current_state;
        
        void ChangeState( StateID new_state );
	};
}