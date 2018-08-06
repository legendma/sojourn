#include "pch.hpp"

#include "common/engine/engine_utilities.hpp"
#include "network_address.hpp"


Engine::NetworkAddress::NetworkAddress( uint32_t in_address, uint16_t port )
{
    // Use IPv4
    GetAddressIn()->sin_family = AF_INET;
    GetAddressIn()->sin_addr.S_un.S_addr = htonl( in_address );
    GetAddressIn()->sin_port = htons( port );
}

Engine::NetworkAddress::NetworkAddress( const sockaddr &other )
{
    m_address = other;
}

std::wstring Engine::NetworkAddress::Print()
{
    auto address = ntohl( GetAddressIn()->sin_addr.S_un.S_addr );
    auto port = ntohs( GetAddressIn()->sin_port );
    
    std::wstring out;
    int number;
    number = (address >> 24) & 0xff;
    out.append( std::to_wstring( number ).append( L"." ) );
    number = (address >> 16) & 0xff;
    out.append( std::to_wstring( number ).append( L"." ) );
    number = (address >> 8) & 0xff;
    out.append( std::to_wstring( number ).append( L"." ) );
    number = (address >> 0) & 0xff;
    out.append( std::to_wstring( number ).append( L":" ) );
    out.append( std::to_wstring( port ) );
     
    return out;
}

boolean Engine::NetworkAddress::Matches( const NetworkAddress &other )
{
    NetworkAddress mine = *this;
    NetworkAddress theirs = other;

    return( mine.GetAddressIn()->sin_addr.S_un.S_addr == theirs.GetAddressIn()->sin_addr.S_un.S_addr
         && mine.GetAddressIn()->sin_port             == theirs.GetAddressIn()->sin_port
         && mine.GetAddressIn()->sin_family           == theirs.GetAddressIn()->sin_family );
}

Concurrency::task<Engine::NetworkAddressPtr> Engine::NetworkAddressFactory::CreateAddressFromStringAsync( const std::wstring address_string )
{
    using namespace Concurrency;
    auto address = std::make_shared<std::wstring>( address_string.c_str() );
    return create_task( [address]() -> Engine::NetworkAddressPtr
    {
        auto position = address->find_last_of( L":" );
        std::wstring node = address->c_str();
        std::wstring service = L"0";
        if( position != std::wstring::npos )
        {
            node = address->substr( 0, position );
            service = address->substr( position + 1 );
        }
    
        ADDRINFOW hints = {};
        hints.ai_family = AF_INET;

        ADDRINFOW *results;
        auto error = GetAddrInfoW( node.c_str(), service.c_str(), &hints, &results );

        if( error != 0
         && results != nullptr )
        {
            Engine::Log( Engine::LOG_LEVEL_ERROR, L"NetworkAddressFactory::CreateAddressFromString" );
            FreeAddrInfoW( results );
            return( nullptr );
        }

        auto walk = results;
        while( walk->ai_addr == 0
            && walk->ai_next )
        {
            walk = walk->ai_next;
        }

        if( walk->ai_addr == 0 )
        {
            FreeAddrInfoW( results );
            return(nullptr);
        }

        auto net_address = NetworkAddressPtr( new NetworkAddress( *walk->ai_addr ) );
        FreeAddrInfoW( results );

        return( net_address );
    } );
}

Engine::NetworkAddressPtr Engine::NetworkAddressFactory::CreateFromAddress( sockaddr &in )
{
    return NetworkAddressPtr( new NetworkAddress( in ) );
}
