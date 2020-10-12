#include "pch.h"

#include "Engine.h"
#include "Window.h"

Engine::Engine( HINSTANCE instance ) :
    window( instance, 1024, 768, keyboard, mouse, graphics )
{
}

void Engine::Run()
{
    // Initialize the graphics driver
    if( !graphics.Initialize( window.GetHwnd(), window.GetWidth(), window.GetHeight() ) )
    {
        return;
    }

    if( show_intro )
    {
        ChangeState( GameStateType::GAME_STATE_INTRO );
    }
    else
    {
        ChangeState( GameStateType::GAME_STATE_MAIN_MENU );
    }
    
    // Run the engine
    while( window.ProcessMessages() )
    {
        Tick();
    }
}

void Engine::Tick()
{
    // TODO <MPA>: Add timers here
    float timestep = 0.0f;

    for( auto &world : worlds )
    {
        systems.Tick( timestep, *world );
    }

    // TODO <MPA>: Do stuff here
    while( !keyboard.IsCharBufferEmpty() )
    {
        std::string out = "Char: ";
        out += keyboard.ReadChar();
        out += "\n";
        OutputDebugStringA( out.c_str() );
    }

    while( !keyboard.IsEventBufferEmpty() )
    {
        std::string out = "";
        auto new_event = keyboard.ReadKeyEvent();
        if( new_event.IsPressed() )
        {
            out += "Pressed: ";
        }
        else if( new_event.IsReleased() )
        {
            out += "Released: ";
        }

        out += new_event.GetKey();
        out += "\n";
        OutputDebugStringA( out.c_str() );
    }

    while( !mouse.IsEventBufferEmpty() )
    {
        std::string out = "";
        auto new_event = mouse.ReadEvent();
        if( new_event.IsLeftPressed() )
        {
            out += "Left Mouse Pressed: ";
        }
        else if( new_event.IsLeftReleased() )
        {
            out += "Left Mouse Released: ";
        }
        else if( new_event.IsRightPressed() )
        {
            out += "Right Mouse Pressed: ";
        }
        else if( new_event.IsRightReleased() )
        {
            out += "Right Mouse Released: ";
        }
        else if( new_event.IsMiddlePressed() )
        {
            out += "Middle Mouse Pressed: ";
        }
        else if( new_event.IsMiddleReleased() )
        {
            out += "Middle Mouse Released: ";
        }
        else if( new_event.IsWheelUp() )
        {
            out += "Mouse Wheel Up: ";
        }
        else if( new_event.IsWheelDown() )
        {
            out += "Mouse Wheel Down: ";
        }
        else if( new_event.IsMove() )
        {
            out += "Mouse Moved: ";
        }
        else if( new_event.IsRawMove() )
        {
            out += "Raw Mouse Moved: ";
        }

        out += std::to_string( new_event.GetX() );
        out += ", ";
        out += std::to_string( new_event.GetY() );
        out += "\n";
        OutputDebugStringA( out.c_str() );

    }
}

void Engine::AddDefaultEntityWorld()
{
    // TODO <MPA>: This is where the initial EntityWorld is created and pushed to the list of active worlds
}

void Engine::AddWorld( EntityWorld * world )
{
}

void Engine::RemoveWorld( EntityWorld * world )
{
}
