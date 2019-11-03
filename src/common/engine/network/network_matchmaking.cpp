#include "pch.hpp"

#include "network_matchmaking.hpp"

void Engine::NetworkConnectionPassport::Write( NetworkConnectionPassportRaw &raw )
{
    ::ZeroMemory( &raw, sizeof( raw ) );
    auto out = Engine::BitStreamFactory::CreateOutputBitStream( reinterpret_cast<byte*>(&raw), raw.size(), false );
    assert( server_address_cnt > 0 );
    assert( server_address_cnt <= (int)server_addresses.size() );

    out->WriteBytes( (void*)NETWORK_PROTOCOL_VERSION, NETWORK_PROTOCOL_VERSION_LEN );
    out->Write( protocol_id );
    out->Write( token_create_time );
    out->Write( token_expire_time );
    out->Write( token_sequence );
    out->Write( timeout_seconds );
    out->Write( server_address_cnt );

    for( auto i = 0; i < server_address_cnt; i++ )
    {
        out->Write( server_addresses[ i ] );
    }

    out->Write( client_to_server_key );
    out->Write( server_to_client_key );

    out->WriteBytes( raw_token.data(), raw_token.size() );
}

bool Engine::NetworkConnectionPassport::Read( NetworkConnectionPassportRaw &raw )
{
    auto in = Engine::BitStreamFactory::CreateInputBitStream( reinterpret_cast<byte*>(&raw), raw.size(), false );

    in->WriteBytes( version.data(), NETWORK_PROTOCOL_VERSION_LEN );
    in->Write( protocol_id );
    in->Write( token_create_time );
    in->Write( token_expire_time );
    in->Write( token_sequence );
    in->Write( timeout_seconds );
    in->Write( server_address_cnt );

    if( server_address_cnt <= 0
     || server_address_cnt > (int)server_addresses.size() )
    {
        return false;
    }

    for( auto i = 0; i < server_address_cnt; i++ )
    {
        in->Write( server_addresses[i] );
    }

    in->Write( client_to_server_key );
    in->Write( server_to_client_key );

    in->WriteBytes( raw_token.data(), raw_token.size() );

    return true;
}

/* TODO <MPA>: REPLACE THIS WITH A MATCHMAKING SERVER THAT GIVES THIS TO CLIENT */
#include <ctime>
#define EXPIRE_DURATION     ( 30 )
#define TIMEOUT_DURATION    ( 5 )
#define SEQUENCE_NUM        ( 0 )
 bool Engine::FAKE_NetworkGetPassport( Engine::NetworkConnectionPassportRaw &out )
{
    ::ZeroMemory( out.data(), out.size() );
    auto address = Engine::NetworkAddressFactory::CreateAddressFromStringAsync( L"127.0.0.1:48000" ).get();
    if( !address )
    {
        return false;
    }

    /* create a fake passport for testing/development purposes - this will be replaced by a HTTPS packet from the matchmaking server */
    Engine::NetworkConnectionPassport passport;
    passport.protocol_id = NETWORK_SOJOURN_PROTOCOL_ID;
    passport.token_create_time = Engine::Time::GetSystemTime();
    passport.token_expire_time = passport.token_create_time + EXPIRE_DURATION;
    passport.token_sequence = SEQUENCE_NUM;
    passport.timeout_seconds = TIMEOUT_DURATION;
    passport.server_address_cnt = 1;
    passport.server_addresses[ 0 ] = address->Address();

    Engine::Networking::GenerateEncryptionKey( passport.client_to_server_key );
    Engine::Networking::GenerateEncryptionKey( passport.server_to_client_key );

    /* create the connection token */
    NetworkConnectionToken token;
    Engine::Networking::GenerateRandom( reinterpret_cast<byte*>(&token.client_id), sizeof( token.client_id ) );
    token.timeout_seconds = passport.timeout_seconds;
    token.server_address_cnt = passport.server_address_cnt;
    token.server_addresses[0] = passport.server_addresses[0];
    token.client_to_server_key = passport.client_to_server_key;
    token.server_to_client_key = passport.server_to_client_key;
    Engine::Networking::GenerateRandom( token.fuzz.data(), token.fuzz.size() );

    token.Write( passport.raw_token );
    if( !NetworkConnectionToken::Encrypt( passport.raw_token, passport.token_sequence, passport.protocol_id, passport.token_expire_time, Engine::SOJOURN_PRIVILEGED_KEY ) )
    {
        return false;
    }

    passport.Write( out );
    return true;
}