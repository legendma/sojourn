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
        Engine::ReportError( L"Networking system could not be started.  Exiting..." );
        return -1;
    }

    auto server = Server::ServerFactory::CreateServer( config, networking );
    if( server == nullptr )
    {
        Engine::ReportError( L"Server was unable to be created.  Exiting..." );
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
    m_next_challenge_sequence( 1 )
{
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
        Engine::ReportError( L"Server::Initialize Given server address is invalid!" );
        throw std::runtime_error( "CreateAddressFromStringAsync" );
    }

    // Create the server socket
    m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_server_address, m_config.receive_buff_size, m_config.send_buff_size );
    if( m_socket == nullptr )
    {
        Engine::ReportError( L"Server::Initialize Unable to create server socket." );
        throw std::runtime_error( "CreateUDPSocket" );
    }

    Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server listening on %s" ).c_str(), m_server_address->Print().c_str() );

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
    ReceivePackets();
    KeepClientsAlive(); // TODO IMPLEMENT
    CheckClientTimeouts(); // TODO IMPLEMENT
    HandleGamePacketsFromClients(); // TODO IMPLEMENT
    RunGameSimulation(); // TODO IMPLEMENT
    SendPacketsToClients(); // TODO IMPLEMENT
}

void Server::Server::ReceivePackets()
{
    Engine::NetworkPacketTypesAllowed allowed;
    allowed.SetAllowed( Engine::PACKET_CONNECT_REQUEST );
    allowed.SetAllowed( Engine::PACKET_CONNECT_CHALLENGE_RESPONSE );
    allowed.SetAllowed( Engine::PACKET_KEEP_ALIVE );
    allowed.SetAllowed( Engine::PACKET_PAYLOAD );
    allowed.SetAllowed( Engine::PACKET_DISCONNECT );

    auto now = m_timer.GetTotalTicks();
    byte data[NETWORK_MAX_PACKET_SIZE];
    while( true )
    {
        Engine::NetworkAddressPtr from;
        auto byte_cnt = m_socket->ReceiveFrom( data, sizeof( data ), from );
        if( byte_cnt == 0 )
            break;

        auto read = Engine::InputBitStreamFactory::CreateInputBitStream( data, byte_cnt, false );
        ReadAndProcessPacket( m_config.protocol_id, allowed, from, now, read );
    }
}

void Server::Server::ReadAndProcessPacket( uint64_t protocol_id, Engine::NetworkPacketTypesAllowed &allowed, Engine::NetworkAddressPtr &from, uint64_t &now_time, Engine::InputBitStreamPtr &read )
{
    Engine::NetworkCryptoMapPtr crypto;
    auto client = FindClientByAddress( from );
    if( client )
    {
        crypto = m_networking->FindCryptoMapByID( client->crypto_id, from, now_time );
    }
    else
    {
        crypto = m_networking->FindCryptoMapByAddress( from, now_time );
    }

    auto marker = read->SaveCurrentLocation();
    Engine::NetworkPacketPrefix prefix;
    read->Read( prefix.b );
    read->SeekToLocation( marker );

    if( prefix.packet_type != Engine::PACKET_CONNECT_REQUEST
     && !crypto )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server packet ignored.  No encryption mapping exists for %s." ).c_str(), from->Print().c_str() );
        return;
    }

    auto packet = m_networking->ReadPacket( protocol_id, crypto, m_config.privileged_key, allowed, now_time, read );
    if( packet )
    {
        ProcessPacket( packet, from, now_time );
    }
}

void Server::Server::ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time )
{
    switch( packet->packet_type )
    {
        case Engine::PACKET_CONNECT_REQUEST:
            OnReceivedConnectionRequest( packet, from, now_time );
            break;
    }
}

void Server::Server::OnReceivedConnectionRequest( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time )
{
    auto request = reinterpret_cast<Engine::NetworkConnectionRequestPacket&>(*packet);
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
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  Server Address not found in whitelist." ).c_str() );
        return;
    }

    if( FindClientByAddress( from ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  Client is already connected." ).c_str() );
        return;
    }

    if( FindClientByClientID( connect_token->client_id ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  Client with same client ID is already connected." ).c_str() );
        return;
    }

    if( !m_seen_tokens.FindAdd( connect_token->authentication, from, m_timer.GetElapsedSeconds() ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Connection Request ignored.  This token has already been seen from a client with a different address." ).c_str() );
        return;
    }

    if( m_clients.size() == m_config.max_num_clients )
    {
        auto refusal = Engine::NetworkPacketFactory::CreateOutgoingConnectionDenied();
        m_networking->SendPacket( m_socket, from, refusal, m_config.protocol_id, connect_token->server_to_client_key );
        return;
    }

    /* send client a connection challenge */
    uint64_t expire_time = 0;
    if( connect_token->timeout_seconds > 0 )
    {
        expire_time = now_time + connect_token->timeout_seconds;
    }

    m_networking->AddCryptoMap( from, connect_token->server_to_client_key, connect_token->client_to_server_key, now_time, expire_time, connect_token->timeout_seconds );

    Engine::NetworkPacketConnectionChallengeHeader challenge;
    challenge.prefix.packet_type = Engine::PACKET_CONNECT_CHALLENGE;
    auto challenge_sequence = m_next_challenge_sequence++;
    challenge.prefix.sequence_byte_cnt = Engine::BitStreamBase::BytesRequired( challenge_sequence );
    challenge.token_sequence = challenge_sequence;

    Engine::NetworkChallengeToken challenge_token;
    challenge_token.client_id = connect_token->client_id;
    challenge_token.authentication = connect_token->authentication;
    Engine::NetworkChallengeTokenRaw raw_challenge_token;
    challenge_token.Write( raw_challenge_token );
    Engine::NetworkChallengeToken::Encrypt( raw_challenge_token, challenge_sequence, m_config.challenge_key );

    auto challenge_packet = Engine::NetworkPacketFactory::CreateOutgoingConnectionChallenge( challenge, raw_challenge_token );
    m_networking->SendPacket( m_socket, from, challenge_packet, m_config.protocol_id, connect_token->server_to_client_key );
    Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server Sent a connection challenge to %s." ).c_str(), from->Print().c_str() );
}

void Server::Server::KeepClientsAlive()
{
}

void Server::Server::CheckClientTimeouts()
{
}

void Server::Server::HandleGamePacketsFromClients()
{
}

void Server::Server::RunGameSimulation()
{
}

void Server::Server::SendPacketsToClients()
{
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

bool Server::SeenTokens::FindAdd( Engine::NetworkAuthentication &token_uid, Engine::NetworkAddressPtr address, double time )
{
    /* first search for the token id in our entries */
    EntryType *match = nullptr;
    EntryType *oldest = nullptr;
    for( size_t i = 0; i < m_seen.size(); i++ )
    {
        if( 0 == std::memcmp( m_seen[ i ].token_uid.data(), token_uid.data(), sizeof( Engine::NetworkAuthentication ) ) )
        {
            match = &m_seen[ i ];
        }

        if( !oldest
         || m_seen[ i ].time < oldest->time )
        {
            oldest = &m_seen[ i ];
        }
    }

    if( !match )
    {
        /* didn't find a match for the given token UID, so overwrite the oldest entry with this token */
        oldest->address = address;
        oldest->time = time;
        std::memcpy( oldest->token_uid.data(), token_uid.data(), sizeof( Engine::NetworkAuthentication ) );
        return true;
    }

    /* check to make sure the token entry we found has come from the same address */
    if( match->address->Matches( *address ) )
    {
        return true;
    }

    return false;
}
