// server_main.cpp : Defines the entry point for the console application.
//

#include "pch.hpp"

#include "common/network/network_main.hpp"
#include "common/engine/engine_utilities.hpp"

#include "server_main.hpp"

int wmain( int argc, wchar_t* argv[] )
{
    const wchar_t *splash = L"\n"
    L"     .d8888b.            d8b                                         \n"
    L"     d88P  Y88b           Y8P                                        \n"
    L"     Y88b.                                                           \n"
    L"      `Y888b.    .d88b.  8888  .d88b.  888  888 888d888 88888b.      \n"
    L"         `Y88b. d88`'88b `888 d88'`88b 888  888 888P'   888 `88b     \n"
    L"           `888 888  888  888 888  888 888  888 888     888  888     \n"
    L"     Y88b  d88P Y88..88P  888 Y88..88P Y88b 888 888     888  888     \n"
    L"      `Y8888P'   `Y88P'   888  `Y88P'   `Y88888 888     888  888     \n"
    L"                          888                                        \n"
    L"            .d8888b.     d88P                                        \n"
    L"           d88P  Y88b  888P'                                         \n"
    L"           Y88b.                                                     \n"
    L"            `Y888b.    .d88b.  888d888 888  888  .d88b.  888d888     \n"
    L"               `Y88b. d8P  Y8b 888P'   888  888 d8P  Y8b 888P'       \n"
    L"                 `888 88888888 888     Y88  88P 88888888 888         \n"
    L"           Y88b  d88P Y8b.     888      Y8bd8P  Y8b.     888         \n"
    L"            `Y8888P'   `Y8888  888       Y88P    `Y8888  888         \n\n"
    L"---------------------------------------------------------------------\n\n";                                                             
                                                            
    wprintf( splash );
    wprintf( L"Starting Server...\n" );
                                                            
    Engine::SetLogLevel( Engine::LOG_LEVEL_DEBUG );

    Server::ServerConfig config;
    config.server_address = std::wstring( L"127.0.0.1:48000" );
    if( argc == 2 )
    {
        config.server_address = std::wstring( argv[ 1 ] );
    }

    auto networking = Engine::NetworkingFactory::StartNetworking();
    if( networking == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Networking system could not be started.  Exiting..." );
        return -1;
    }

    auto server = Server::ServerFactory::CreateServer( config, networking );
    if( server == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Server was unable to be created.  Exiting..." );
        return -1;
    }

    server->Run();

    wprintf( L"Shutting down\n" );

    server.reset();
    networking.reset();

    return 0;
}

Server::Server::Server( ServerConfig &config, Engine::NetworkingPtr &networking ) : 
    m_networking( networking ),
    m_quit( false ),
    m_config( config ),
    m_next_sequence( 1 ),
    m_next_challenge_sequence( 1 )
{
    m_now_time = m_timer.GetTotalSeconds();
    Initialize();
}

Server::Server::~Server()
{
}

void Server::Server::Initialize()
{
    // Create the server address
    m_server_address = Engine::NetworkAddressFactory::CreateAddressFromStringAsync( m_config.server_address ).get();
    if( m_server_address == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Server::Initialize Given server address is invalid!" );
        throw std::runtime_error( "CreateAddressFromStringAsync" );
    }

    // Create the server socket
    m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_server_address, m_config.receive_buff_size, m_config.send_buff_size );
    if( m_socket == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Server::Initialize Unable to create server socket." );
        throw std::runtime_error( "CreateUDPSocket" );
    }

    Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server listening on %s", m_server_address->Print().c_str() );

    // setup the server timer
    m_timer.SetFixedTimeStep( true );
    m_timer.SetTargetElapsedSeconds( 1.0 / m_config.server_fps );
}

void Server::Server::Run()
{
    m_timer.ResetElapsedTime();
    while( !m_quit )
    {
        m_timer.Tick( [&]()
        {
            Update();
        } );
    }
}

void Server::Server::Update()
{
    m_now_time = m_timer.GetTotalSeconds();

    ReceivePackets();
    CheckClientTimeouts(); // TODO IMPLEMENT
    HandleGamePacketsFromClients(); // TODO IMPLEMENT
    RunGameSimulation(); // TODO IMPLEMENT
    SendGamePacketsToClients(); // TODO IMPLEMENT
    KeepClientsAlive(); // TODO IMPLEMENT
}

void Server::Server::ReceivePackets()
{
    Engine::NetworkPacketTypesAllowed allowed;
    allowed.SetAllowed( Engine::PACKET_CONNECT_REQUEST );
    allowed.SetAllowed( Engine::PACKET_CONNECT_CHALLENGE_RESPONSE );
    allowed.SetAllowed( Engine::PACKET_KEEP_ALIVE );
    allowed.SetAllowed( Engine::PACKET_PAYLOAD );
    allowed.SetAllowed( Engine::PACKET_DISCONNECT );

    byte data[NETWORK_MAX_PACKET_SIZE];
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

void Server::Server::ReadAndProcessPacket( uint64_t protocol_id, Engine::NetworkPacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, Engine::InputBitStreamPtr &read )
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
    read->Read( prefix.b );
    read->SeekToLocation( marker );

    if( prefix.packet_type != Engine::PACKET_CONNECT_REQUEST
     && !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server packet ignored.  No encryption mapping exists for %s.", from->Print().c_str() );
        return;
    }

    auto packet = Engine::NetworkPacket::ReadPacket( read, allowed, protocol_id, crypto->receive_key, m_now_time );
    if( packet )
    {
        ProcessPacket( packet, from );
    }
}

void Server::Server::ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from )
{
    switch( packet->packet_type )
    {
        case Engine::PACKET_CONNECT_REQUEST:
            OnReceivedConnectionRequest( reinterpret_cast<Engine::NetworkConnectionRequestPacket&>(*packet), from );
            break;

        case Engine::PACKET_CONNECT_CHALLENGE_RESPONSE:
            OnReceivedConnectionChallengeResponse( reinterpret_cast<Engine::NetworkConnectionChallengeResponsePacket&>(*packet), from );
            break;

        case Engine::PACKET_KEEP_ALIVE:
            OnReceivedKeepAlive( reinterpret_cast<Engine::NetworkKeepAlivePacket&>(*packet), from );
            break;
                
        case Engine::PACKET_PAYLOAD:
            break;

        case Engine::PACKET_DISCONNECT:
            break;
    }
}

void Server::Server::OnReceivedConnectionRequest( Engine::NetworkConnectionRequestPacket &request, Engine::NetworkAddressPtr &from )
{
    auto &connect_token = request.token;
    bool found_our_address = false;
    for( auto i = 0; i < connect_token->server_address_cnt; i++ )
    {
        auto address = Engine::NetworkAddressFactory::CreateFromAddress( connect_token->server_addresses[i] );
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
        auto refusal = Engine::NetworkPacketFactory::CreateConnectionDenied();
        (void)m_networking->SendPacket( m_socket, from, refusal, m_config.protocol_id, connect_token->server_to_client_key, m_next_sequence++ );
        return;
    }

    /* send client a connection challenge */
    double expire_time = 0;
    if( connect_token->timeout_seconds > 0 )
    {
        expire_time = m_now_time + connect_token->timeout_seconds;
    }

    m_networking->AddCryptoMap( from, connect_token->server_to_client_key, connect_token->client_to_server_key, m_now_time, expire_time, connect_token->timeout_seconds );

    Engine::NetworkConnectionChallengeHeader challenge;
    challenge.token_sequence = m_next_challenge_sequence++;

    Engine::NetworkChallengeToken challenge_token;
    challenge_token.client_id = connect_token->client_id;
    challenge_token.authentication = connect_token->authentication;
    challenge_token.Write( challenge.raw_challenge_token );
    Engine::NetworkChallengeToken::Encrypt( challenge.raw_challenge_token, challenge.token_sequence, m_config.challenge_key );

    auto challenge_packet = Engine::NetworkPacketFactory::CreateConnectionChallenge( challenge );
    if( m_networking->SendPacket( m_socket, from, challenge_packet, m_config.protocol_id, connect_token->server_to_client_key, challenge.token_sequence ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server unable to send a connection challenge to %s.", from->Print().c_str() );
        return;
    }

    Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Sent a connection challenge to %s.", from->Print().c_str() );
}

void Server::Server::OnReceivedConnectionChallengeResponse( Engine::NetworkConnectionChallengeResponsePacket &response, Engine::NetworkAddressPtr &from )
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

    auto crypto = m_networking->FindCryptoMapByClientID( response.token->client_id, from, m_now_time );
    if( !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response ignored.  Could not find client cryptographic credentials." );
        return;
    }

    if( m_clients.size() == m_config.max_num_clients )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Connection Challenge response denied.  Server is full. Sending denied response..." );
        auto refusal = Engine::NetworkPacketFactory::CreateConnectionDenied();
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
    m_clients.push_back( new_client );

    /* let the client know the connection was accepted by sending a keep alive packet */
    auto packet = Engine::NetworkPacketFactory::CreateKeepAlive( response.token->client_id, m_config.max_num_clients );
    (void)SendClientPacket( response.token->client_id, packet );
}

void Server::Server::OnReceivedKeepAlive( Engine::NetworkKeepAlivePacket &keep_alive, Engine::NetworkAddressPtr &from )
{
    auto crypto = m_networking->FindCryptoMapByClientID( keep_alive.header.client_id, from, m_now_time );
    if( !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Keep Alive ignored.  Could not find client cryptographic credentials." );
        return;
    }
    
    auto client = FindClientByClientID( keep_alive.header.client_id );
    if( !client )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Server Keep Alive ignored.  Could not find a matching client." );
        return;
    }

    client->is_confirmed = true;
    client->last_time_received_packet = m_now_time;

}

void Server::Server::KeepClientsAlive()
{
    for( auto client : m_clients )
    {
        if( m_now_time - client->last_time_sent_packet < m_config.send_rate )
        {
            continue;
        }

        auto packet = Engine::NetworkPacketFactory::CreateKeepAlive( client->client_id, m_config.max_num_clients );
        if( !SendClientPacket( client->client_id, packet ) )
        {
            Engine::Log( Engine::LOG_LEVEL_WARNING, L"Server::KeepClientsAlive not able to send client %d keep alive packet.", client->client_id );
        }
    }
}

void Server::Server::CheckClientTimeouts()
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

void Server::Server::DisconnectClient( uint64_t client_id, int num_of_disconnect_packets /*=SERVER_NUM_OF_DISCONNECT_PACKETS*/ )
{
    ClientRecordPtr client;
    auto it = m_clients.begin();
    for( ; it != m_clients.end(); it++ )
    {
        if( (*it)->client_id == client_id )
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
        auto packet = Engine::NetworkPacketFactory::CreateDisconnect();
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

void Server::Server::HandleGamePacketsFromClients()
{
}

void Server::Server::RunGameSimulation()
{
}

void Server::Server::SendGamePacketsToClients()
{
    // TODO <MPA>: This needs to send a keep alive packet to each client every frame, before any payload packet is sent - until
    //             the client's connection is 'confirmed'.
}

bool Server::Server::SendClientPacket( uint64_t client_id, Engine::NetworkPacketPtr &packet )
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

Server::ClientRecordPtr Server::Server::FindClientByAddress( Engine::NetworkAddressPtr &search )
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

Server::ClientRecordPtr Server::Server::FindClientByClientID( uint64_t search )
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

Server::ServerPtr Server::ServerFactory::CreateServer( ServerConfig &config, Engine::NetworkingPtr &networking )
{
    try
    {
        return ServerPtr( new Server( config, networking ) );
    }
    catch( std::runtime_error() ) {}

    return nullptr;
}

Server::SeenTokens::SeenTokens()
{
    for( size_t i = 0; i < m_seen.size(); i++ )
    {
        m_seen[ i ].time = 0.0;
        memset( m_seen[ i ].token_uid.data(), 0, sizeof( Engine::NetworkKey ) );
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
            *oldest = m_seen[i];
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
