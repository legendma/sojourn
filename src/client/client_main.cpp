#include "pch.hpp"

#include "common/app/app_sojourn.hpp"
#include "app/app_client.hpp"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd )
{
    auto app = Application::Application<Client::Application>();
    return app.Run( hInstance );
}