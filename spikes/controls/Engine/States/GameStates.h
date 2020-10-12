#pragma once
#include "IEngine.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"

enum class GameStateType
{
    GAME_STATE_INTRO,
    GAME_STATE_MAIN_MENU,
    GAME_STATE_IN_GAME
};

class GameStateBase
{
public:   
    GameStateBase( GameStateType state_name, IEngine &engine ) : engine( engine ), state_name( state_name ), keyboard( engine.GetKeyboard() ), mouse( engine.GetMouse() ), graphics( engine.GetGraphics() ) {}
    virtual void OnEnterState() {}
    virtual void OnExitState() {}
    virtual void Tick( float timestep ) {}

    GameStateType state_name;
    Keyboard &keyboard;
    Mouse &mouse;
    Graphics &graphics;
    IEngine &engine;
};