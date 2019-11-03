#pragma once

#include "common/engine/engine_step_timer.hpp"
#include "common/engine/network/network_main.hpp"
#include "common/engine/network/network_reliable_endpoint.hpp"
#include "common/engine/network/network_matchmaking.hpp"
#include "engine/network/network_client_config.hpp"

namespace Engine
{
	class NetworkConnection
	{
        friend class NetworkConnectionFactory;
        friend class DisconnectedState;
        friend class TryNextServerState;
        friend class SendingConnectRequestsState;
        friend class SendingConnectRepliesState;
        friend class ConnectedState;
        friend class DisconnectingState;
	public:
        typedef enum
        {
            DISCONNECTED,
            TRY_NEXT_SERVER,
            SENDING_CONNECT_REQUESTS,
            SENDING_CONNECT_REPLIES,
            CONNECTED,
            DISCONNECTING
        } StateID;

        typedef enum
        {
            NO_CONNECTION_ERROR,
            SERVER_SOCKET_ERROR,
            SERVER_NOT_RESPONDING,
            CANT_REACH_SERVER,
            SERVER_IS_FULL,
            SERVER_DISCONNECTED,
            SYSTEM_ERROR
        } ConnectionError;

        interface IState
        {
        public:
            virtual void EnterState() = 0;
            virtual void ExitState() = 0;
            virtual void Update() = 0;
            virtual void ProcessPacket( Engine::NetworkPacketPtr &packet ) = 0;
            virtual StateID GetId() = 0;
        }; typedef std::shared_ptr<IState> StatePtr;

        template<StateID _type_id>
        struct State : public IState
        {
            State( NetworkConnection &fsm ) : m_fsm( fsm ) {};
            StateID GetId() { return id; }

            static const StateID id = _type_id;
            NetworkConnection &m_fsm;
        };

        void SendAndReceivePackets();
        void Connect( NetworkConnectionPassportPtr &offer );

        bool IsConnected() { return m_current_state->GetId() == CONNECTED; }
        ConnectionError GetConnectionError() { return m_connect_error; }
        bool NeedsMoreServers();
        NetworkMessagePtr PopIncomingMessage();

    protected:
        std::map<StateID, StatePtr> m_states;
        StatePtr m_current_state;
        ConnectionError m_connect_error;
        NetworkSocketUDPPtr m_socket;
        NetworkingPtr m_networking;
        NetworkReliableEndpointPtr m_endpoint;
        NetworkAddressPtr m_server_address;
        NetworkAddressPtr m_our_address;
        NetworkConnectionPassportPtr m_passport;
        NetworkPacketTypesAllowed m_allowed;
        const NetworkClientConfig &m_config;
        StepTimer m_send_timer;
        uint64_t m_send_packet_sequence;
        double m_connect_start_time;
        double m_current_time;
        double m_last_recieved_packet_time;
        double m_last_sent_packet_time;
        NetworkConnectionChallengeHeader m_challenge;
        NetworkPacketPtr m_keep_alive_packet;
        uint64_t m_client_id;
        
        NetworkConnection( const NetworkClientConfig &config, NetworkingPtr &networking );
        void ChangeState( StateID to_state );
        bool IsPassportExpired();
        void ReceiveAndProcessPackets();

        inline void ResetSendTimer() { m_send_timer = Engine::StepTimer(); }

	}; typedef std::shared_ptr<NetworkConnection> NetworkConnectionPtr;

    class NetworkConnectionFactory
    {
    public:
        static NetworkConnectionPtr CreateConnection( const NetworkClientConfig &config, NetworkingPtr &networking );
    };
}