#include "pch.hpp"
#include "network_main.hpp"


Engine::NetworkMain::NetworkMain()
{
    WSAStartup( MAKEWORD( 2, 2 ), &m_wsa_data );
}


Engine::NetworkMain::~NetworkMain()
{
}
