#include "pch.hpp"

#include "common/app/app_sojourn.hpp"
#include "app/app_server.hpp"

int wmain( int argc, wchar_t* argv[] )
{
    auto server_address = std::wstring( L"127.0.0.1:48000" );
    if( argc == 2 )
    {
        server_address = std::wstring( argv[ 1 ] );
    }

    auto app = Application::Application<Server::Application>();
    return app.Run( server_address );

    //server->Run();

    //server.reset();

    return 0;
}