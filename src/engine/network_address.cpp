#include "pch.hpp"

#include "engine_utilities.hpp"
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
            Engine::ReportError( L"NetworkAddressFactory::CreateAddressFromString" );
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