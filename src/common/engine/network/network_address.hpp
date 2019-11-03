#pragma once

namespace Engine
{
    class NetworkAddress
    {
        friend class NetworkSocketUDP;
        friend class NetworkSocketTCP;
    public:
        NetworkAddress( uint32_t in_address, uint16_t port );
        NetworkAddress( const sockaddr &other );

        size_t GetSize() const { return sizeof( sockaddr ); }
        sockaddr Address() { return m_address; }
        std::wstring Print();
        int Port();
        boolean Matches( const NetworkAddress &other );

    private:
        sockaddr m_address;

        sockaddr_in * GetAddressIn() { return reinterpret_cast<sockaddr_in*>( &m_address ); }
    }; typedef std::shared_ptr<NetworkAddress> NetworkAddressPtr;

    class NetworkAddressFactory
    {
    public:
        static Concurrency::task<Engine::NetworkAddressPtr> CreateAddressFromStringAsync( const std::wstring address_string );
        static NetworkAddressPtr CreateFromAddress( sockaddr &in );
    };
}

