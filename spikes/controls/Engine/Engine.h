#pragma once

#include "IEngine.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"
#include "Window.h"
#include "EntityWorld.h"
#include "SystemsManager.h"
#include "GameState.h"

class Engine : public IEngine
{
public:
    Engine( HINSTANCE instance );
    void Run();
    void Tick();
    void AddDefaultEntityWorld();

    // IEngine
    virtual Keyboard & GetKeyboard() { return keyboard; }
    virtual Mouse & GetMouse() { return mouse; }
    virtual Graphics & GetGraphics() { return graphics; }
    virtual void ChangeState( const GameStateType new_state ) { state.ChangeState( new_state, *this ); }
    virtual void AddWorld( EntityWorld *world );
    virtual void RemoveWorld( EntityWorld *world );

protected:
    Window window;
    Keyboard keyboard;
    Mouse mouse;
    Graphics graphics;
    SystemsManager systems;
    std::vector<std::shared_ptr<EntityWorld>> worlds;
    GameState state;
    bool show_intro = false;
};

