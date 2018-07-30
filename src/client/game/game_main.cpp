#include "pch.hpp"

#include <ppltasks.h>	// For create_task

#include "game_main.h"

using namespace Game;
using namespace Concurrency;

Main::Main( const std::shared_ptr<Engine::GraphicsAdapter>& graphics, Engine::StepTimer &timer )
         : m_scripting( nullptr ),
           m_graphics( graphics ),
           m_timer( timer )
{
    // Register to be notified if the Device is lost or recreated
    m_graphics->RegisterDeviceNotify( this );

    // TODO move this
    m_sceneRenderer = std::make_unique<World3DRenderer>( m_graphics );
    m_guiRenderer   = std::make_unique<GUIRenderer>( m_graphics );

    StartupSystem();
}

void Main::StartupSystem( void )
{
    auto startup = create_task( [this]
    {
        // Bootstrap Lua
        m_scripting = std::make_shared<Engine::Scripts>();
    } );
}

Main::~Main()
{
    // Deregister device notification
    m_graphics->RegisterDeviceNotify( nullptr );
}


void Main::OnDeviceLost()
{
}

void Main::OnDeviceRestored()
{
}
