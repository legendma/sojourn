#include "pch.hpp"

#include "app_server.hpp"

#include "common/engine/engine_utilities.hpp"


Server::Application::Application( std::wstring server_address ) :
    m_server_address_string( server_address ),
    m_quit( false ),
    m_next_sequence( 1 ),
    m_next_challenge_sequence( 1 )
{
}

void Server::Application::CheckClientTimeouts()
{
    for( auto client : m_clients )
    {
        if( client->timeout_seconds <= 0
            || client->last_time_received_packet + client->timeout_seconds > m_now_time )
        {
            continue;
        }

        /* client has timeout out.  haven't recieved a packet from them in a while.  disconnect them */
        Engine::Log( Engine::LOG_LEVEL_INFO, L"Server found Client ID %d has timed out.", client->client_id );
        DisconnectClient( client->client_id, 0 );
    }
}

void Server::Application::DisconnectClient( uint64_t client_id, int num_of_disconnect_packets /*=SERVER_NUM_OF_DISCONNECT_PACKETS*/ )
{
    ClientRecordPtr client;
    auto it = m_clients.begin();
    for( ; it != m_clients.end(); it++ )
    {
        if( ( *it )->client_id == client_id )
        {
            client = *it;
            break;
        }
    }

    if( !client )
    {
        Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::DisconnectClient was ordered to disconnect, but client ID %d didn't exist.", client_id );
        return;
    }

    if( num_of_disconnect_packets > 0 )
    {
        auto crypto = m_networking->FindCryptoMapByClientID( client_id, client->client_address, m_now_time );
        auto packet = Engine::NetworkPacketFactory::CreateDisconnect( m_networking->AsAllocator() );
        for( auto i = 0; i < num_of_disconnect_packets; i++ )
        {
            m_networking->SendPacket( m_socket, client->client_address, packet, m_config.protocol_id, crypto->send_key, client->client_sequence++ );
        }
    }

    if( !m_networking->DeleteCryptoMapsFromAddress( client->client_address ) )
    {
        Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::DisconnectClient tried to delete cryptographic credentials from client address, but none found!" );
    }

    m_clients.erase( it );
}

Server::ClientRecordPtr Server::Application::FindClientByAddress( Engine::NetworkAddressPtr &search )
{
    assert( m_clients.size() <= m_config.max_num_clients );
    for( auto client : m_clients )
    {
        if( client->client_address->Matches( *search ) )
        {
            return client;
        }
    }

    return nullptr;
}

Server::ClientRecordPtr Server::Application::FindClientByClientID( uint64_t search )
{
    assert( m_clients.size() <= m_config.max_num_clients );
    for( auto client : m_clients )
    {
        if( client && client->client_id == search )
        {
            return client;
        }
    }

    return nullptr;
}

void Server::Application::HandleGamePacketsFromClients()
{
    for( auto client : m_clients )
    {
        if( !client->endpoint->ProcessReceivedPackets( m_now_time ) )
        {
            DisconnectClient( client->client_id );
        }
    }
}

void Server::Application::KeepClientsAlive()
{
    for( auto client : m_clients )
    {
        if( m_now_time - client->last_time_sent_packet < m_config.send_rate / 1000.0f )
        {
            continue;
        }

        auto packet = Engine::NetworkPacketFactory::CreateKeepAlive( m_networking->AsAllocator(), client->client_id );
        if( !SendClientPacket( client->client_id, packet ) )
        {
            Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::KeepClientsAlive not able to send client %d keep alive packet.", client->client_id );
        }
    }
}

void Server::Application::OnReceivedConnectionChallengeResponse( Engine::NetworkConnectionChallengeResponsePacket &response, Engine::NetworkAddressPtr &from )
{
    if( FindClientByAddress( from ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response ignored.  Client is already connected." );
        return;
    }

    if( !Engine::NetworkChallengeToken::Decrypt( response.header.raw_challenge_token, response.header.token_sequence, m_config.challenge_key ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response ignored.  Could not decrypt challenge token." );
        return;
    }

    if( !response.token->Read( response.header.raw_challenge_token ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response ignored.  Could not read challenge token." );
        return;
    }

    if( FindClientByClientID( response.token->client_id ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response ignored.  Client with same client ID is already connected." );
        return;
    }

    auto seen = m_seen_tokens.FindAdd( response.token->authentication, from, m_timer.GetElapsedSeconds() );
    if( !seen )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response ignored.  This token has not been seen from this client before." );
        return;
    }

    auto crypto = m_networking->FindCryptoMapByAddress( from, m_now_time );
    if( !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response ignored.  Could not find client cryptographic credentials." );
        return;
    }

    if( m_clients.size() == m_config.max_num_clients )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response denied.  Server is full. Sending denied response..." );
        auto refusal = Engine::NetworkPacketFactory::CreateConnectionDenied( m_networking->AsAllocator() );
        (void)m_networking->SendPacket( m_socket, from, refusal, m_config.protocol_id, crypto->send_key, m_next_sequence++ );
        return;
    }

    /* add a new connected client */
    auto new_client = ClientRecordPtr( new ClientRecord() );
    new_client->client_address = from;
    new_client->client_id = response.token->client_id;
    new_client->last_time_sent_packet = m_now_time;
    new_client->last_time_received_packet = new_client->last_time_sent_packet;
    new_client->timeout_seconds = crypto->timeout_seconds;
    new_client->endpoint = Engine::NetworkReliableEndpointPtr( new Engine::NetworkReliableEndpoint() );

    m_simulation->AddPlayer( new_client->endpoint );
    m_clients.push_back( new_client );

    Engine::Log( Engine::LOG_LEVEL_INFO, L"Server connected Client ID %d", response.token->client_id );

    /* let the client know the connection was accepted by sending a keep alive packet */
    auto packet = Engine::NetworkPacketFactory::CreateKeepAlive( m_networking->AsAllocator(), response.token->client_id );
    (void)SendClientPacket( response.token->client_id, packet );
}

void Server::Application::OnReceivedConnectionRequest( Engine::NetworkConnectionRequestPacket &request, Engine::NetworkAddressPtr &from )
{
    auto &connect_token = request.token;
    bool found_our_address = false;
    for( auto i = 0; i < connect_token->server_address_cnt; i++ )
    {
        auto address = Engine::NetworkAddressFactory::CreateFromAddress( connect_token->server_addresses[ i ] );
        if( m_server_address->Matches( *address ) )
        {
            found_our_address = true;
        }
    }

    if( !found_our_address )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Request ignored.  Server Address not found in whitelist." );
        return;
    }

    if( FindClientByAddress( from ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Request ignored.  Client is already connected." );
        return;
    }

    if( FindClientByClientID( connect_token->client_id ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Request ignored.  Client with same client ID is already connected." );
        return;
    }

    if( !m_seen_tokens.FindAdd( connect_token->authentication, from, m_timer.GetElapsedSeconds() ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Request ignored.  This token has already been seen from a client with a different address." );
        return;
    }

    if( m_clients.size() == m_config.max_num_clients )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Request denied.  Server is full. Sending denied response..." );
        auto refusal = Engine::NetworkPacketFactory::CreateConnectionDenied( m_networking->AsAllocator() );
        (void)m_networking->SendPacket( m_socket, from, refusal, m_config.protocol_id, connect_token->server_to_client_key, m_next_sequence++ );
        return;
    }

    /* send client a connection challenge */
    double expire_time = 0;
    if( connect_token->timeout_seconds > 0 )
    {
        expire_time = m_now_time + connect_token->timeout_seconds;
    }

    m_networking->AddCryptoMap( connect_token->client_id, from, connect_token->server_to_client_key, connect_token->client_to_server_key, m_now_time, expire_time, connect_token->timeout_seconds );

    Engine::NetworkConnectionChallengeHeader challenge;
    challenge.token_sequence = m_next_challenge_sequence++;

    Engine::NetworkChallengeToken challenge_token;
    challenge_token.client_id = connect_token->client_id;
    challenge_token.authentication = connect_token->authentication;
    challenge_token.Write( challenge.raw_challenge_token );
    Engine::NetworkChallengeToken::Encrypt( challenge.raw_challenge_token, challenge.token_sequence, m_config.challenge_key );

    auto challenge_packet = Engine::NetworkPacketFactory::CreateConnectionChallenge( m_networking->AsAllocator(), challenge );
    if( !m_networking->SendPacket( m_socket, from, challenge_packet, m_config.protocol_id, connect_token->server_to_client_key, challenge.token_sequence ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server unable to send a connection challenge to %s.", from->Print().c_str() );
        return;
    }

    Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Sent a connection challenge to %s.", from->Print().c_str() );
}

void Server::Application::OnReceivedKeepAlive( Engine::NetworkKeepAlivePacket &keep_alive, Server::ClientRecordPtr &client )
{
    if( !client )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Keep Alive ignored.  Could not find a matching client." );
        return;
    }

    auto crypto = m_networking->FindCryptoMapByClientID( keep_alive.header.client_id, client->client_address, m_now_time );
    if( !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Keep Alive ignored.  Could not find client cryptographic credentials." );
        return;
    }

    client->is_confirmed = true;
    client->last_time_received_packet = m_now_time;
}

void Server::Application::ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, Server::ClientRecordPtr &client )
{
    switch( packet->packet_type )
    {
    case Engine::PACKET_CONNECT_REQUEST:
        OnReceivedConnectionRequest( reinterpret_cast<Engine::NetworkConnectionRequestPacket&>( *packet ), from );
        break;

    case Engine::PACKET_CONNECT_CHALLENGE_RESPONSE:
        OnReceivedConnectionChallengeResponse( reinterpret_cast<Engine::NetworkConnectionChallengeResponsePacket&>( *packet ), from );
        break;

    case Engine::PACKET_KEEP_ALIVE:
        OnReceivedKeepAlive( reinterpret_cast<Engine::NetworkKeepAlivePacket&>( *packet ), client );
        break;

    case Engine::PACKET_PAYLOAD:
        client->endpoint->in_queue.push( packet );
        break;

    case Engine::PACKET_DISCONNECT:
        break;
    }
}

void Server::Application::ReadAndProcessPacket( uint64_t protocol_id, Engine::NetworkPacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, Engine::InputBitStreamPtr &read )
{
    Engine::NetworkCryptoMapPtr crypto;
    auto client = FindClientByAddress( from );
    if( client )
    {
        crypto = m_networking->FindCryptoMapByClientID( client->client_id, from, m_now_time );
    }
    else
    {
        crypto = m_networking->FindCryptoMapByAddress( from, m_now_time );
    }

    auto marker = read->SaveCurrentLocation();
    Engine::NetworkPacketPrefix prefix;
    read->Write( prefix.b );
    read->SeekToLocation( marker );

    if( prefix.packet_type != Engine::PACKET_CONNECT_REQUEST
        && !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server packet ignored.  No encryption mapping exists for %s.", from->Print().c_str() );
        return;
    }

    auto packet = Engine::NetworkPacket::ReadPacket( m_networking->AsAllocator(), read, allowed, protocol_id, crypto->receive_key, m_now_time );
    if( packet )
    {
        ProcessPacket( packet, from, client );
    }
}

void Server::Application::ReceivePackets()
{
    Engine::NetworkPacketTypesAllowed allowed;
    allowed.SetAllowed( Engine::PACKET_CONNECT_REQUEST );
    allowed.SetAllowed( Engine::PACKET_CONNECT_CHALLENGE_RESPONSE );
    allowed.SetAllowed( Engine::PACKET_KEEP_ALIVE );
    allowed.SetAllowed( Engine::PACKET_PAYLOAD );
    allowed.SetAllowed( Engine::PACKET_DISCONNECT );

    byte data[ NETWORK_MAX_PACKET_SIZE ];
    while( true )
    {
        Engine::NetworkAddressPtr from;
        auto byte_cnt = m_socket->ReceiveFrom( data, sizeof( data ), from );
        if( byte_cnt == 0 )
            break;

        auto read = Engine::BitStreamFactory::CreateInputBitStream( data, byte_cnt, false );
        ReadAndProcessPacket( m_config.protocol_id, allowed, from, read );
    }
}

int Server::Application::Run()
{
    m_timer.ResetElapsedTime();
    while( !m_quit )
    {
        m_timer.Tick( [&]()
        {
            m_now_time = Engine::Time::GetSystemTime();

            ReceivePackets();
            CheckClientTimeouts();
            HandleGamePacketsFromClients();
            RunGameSimulation();
            SendGamePacketsToClients();
            KeepClientsAlive();
        } );
    }

    return 0;
}

void Server::Application::RunGameSimulation()
{
    // update the simulation
    m_simulation->RunFrame( (float)m_timer.GetElapsedSeconds() );
}

bool Server::Application::SendClientPacket( uint64_t client_id, Engine::NetworkPacketPtr &packet )
{
    auto client = FindClientByClientID( client_id );
    if( !client )
    {
        Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::SendClientPacket could not find the client by given client ID %d", client_id );
        return false;
    }

    auto crypto = m_networking->FindCryptoMapByClientID( client_id, client->client_address, m_now_time );
    if( !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::SendClientPacket could not find client %d 's cryptographic credentials.", client_id );
        return false;
    }

    if( !m_networking->SendPacket( m_socket, client->client_address, packet, m_config.protocol_id, crypto->send_key, client->client_sequence++ ) )
    {
        Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::SendClientPacket failed to send packet to client %d.", client_id );
        return false;
    }

    client->last_time_sent_packet = m_now_time;
    return true;
}

void Server::Application::SendGamePacketsToClients()
{
    for( auto client : m_clients )
    {
        /* if the client is not confirmed connected yet, send a keep alive packet to establish the connection, until we received our first packet from them */
        if( !client->is_confirmed )
        {
            auto packet = Engine::NetworkPacketFactory::CreateKeepAlive( m_networking->AsAllocator(), client->client_id );
            (void)SendClientPacket( client->client_id, packet );
        }

        client->endpoint->PackageOutgoingPackets( m_networking->AsAllocator(), client->client_id, m_now_time );
        while( client->endpoint->out_queue.size() )
        {
            auto &outgoing = client->endpoint->out_queue.front();
            if( !SendClientPacket( client->client_id, outgoing.packet ) )
            {
                Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::SendGamePacketsToClients failed to send packet to client %d.", client->client_id );
            }
            else
            {
                client->endpoint->MarkSent( outgoing, m_now_time );
            }

            client->endpoint->out_queue.pop_front();
        }

    }
}

void Server::Application::Shutdown()
{
    wprintf( L"Shutting down\n" );
    m_simulation.reset();
    m_networking.reset();
}

bool Server::Application::Start()
{
    wprintf( SPLASH );
    wprintf( L"Starting Server...\n" );

    Engine::SetLogLevel( Engine::LOG_LEVEL_INFO );

    m_networking = Engine::NetworkingFactory::StartNetworking();
    if( m_networking == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Networking system could not be started.  Exiting..." );
        return false;
    }

    m_now_time = Engine::Time::GetSystemTime();

    // Create the server address
    m_config.server_address = m_server_address_string;
    m_server_address = Engine::NetworkAddressFactory::CreateAddressFromStringAsync( m_config.server_address ).get();
    if( m_server_address == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Server::Initialize Given server address is invalid!" );
        return false;
    }

    // Create the server socket
    m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_server_address, m_config.receive_buff_size, m_config.send_buff_size );
    if( m_socket == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Server::Initialize Unable to create server socket." );
        return false;
    }

    Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server listening on %s", m_server_address->Print().c_str() );

    // setup the server timer
    m_timer.SetFixedTimeStep( true );
    m_timer.SetTargetElapsedSeconds( 1.0 / m_config.server_fps );

    // create the game simulation
    m_simulation = Game::GameSimulationPtr( new Game::GameSimulation() );

    //auto server = Server::ServerFactory::CreateServer( config, networking );
    //if( server == nullptr )
    //{
    //    Engine::Log( Engine::LOG_LEVEL_ERROR, L"Server was unable to be created.  Exiting..." );
    //    return -1;
    //}

    return true;
}

Server::SeenTokens::SeenTokens()
{
    for( size_t i = 0; i < m_seen.size(); i++ )
    {
        m_seen[ i ].time = 0.0;
        std::memset( m_seen[ i ].token_uid.data(), 0, sizeof( Engine::NetworkKey ) );
    }
}

Server::SeenTokens::EntryTypePtr Server::SeenTokens::FindAdd( Engine::NetworkAuthentication &token_uid, Engine::NetworkAddressPtr address, double time )
{
    /* first search for the token id in our entries */
    EntryTypePtr match;
    EntryTypePtr oldest;
    for( size_t i = 0; i < m_seen.size(); i++ )
    {
        if( 0 == std::memcmp( m_seen[ i ].token_uid.data(), token_uid.data(), sizeof( Engine::NetworkAuthentication ) ) )
        {
            match = EntryTypePtr( new EntryType );
            *match = m_seen[ i ];
        }

        if( !oldest
            || m_seen[ i ].time < oldest->time )
        {
            oldest = EntryTypePtr( new EntryType );
            *oldest = m_seen[ i ];
        }
    }

    if( !match )
    {
        /* didn't find a match for the given token UID, so overwrite the oldest entry with this token */
        oldest->address = address;
        oldest->time = time;
        std::memcpy( oldest->token_uid.data(), token_uid.data(), sizeof( Engine::NetworkAuthentication ) );
        return oldest;
    }

    /* check to make sure the token entry we found has come from the same address */
    if( match->address->Matches( *address ) )
    {
        return match;
    }

    return nullptr;
}
