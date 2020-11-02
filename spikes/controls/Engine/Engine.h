#pragma once

#include "IEngine.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"
#include "Window.h"
#include "EntityWorld.h"
#include "SystemsManager.h"
#include "GameState.h"

class EngineTimer
{
public:
    EngineTimer();
    float GetElapsedMillis();

    inline uint32_t GetFPS() { return frames_per_second; }

private:
    LARGE_INTEGER frequency = {0};
    LARGE_INTEGER last_time = {0};
    uint64_t max_delta = 0;
    uint32_t frame_count = 0;
    uint32_t frames_per_second = 0;
    uint32_t frames_this_second = 0;
    uint64_t second_counter = 0;

    // Integer format represents time using 10,000,000 ticks per second.
    static const uint64_t ticks_per_second = 10000000;
};

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
    EngineTimer timer;
    SystemsManager systems;
    std::vector<std::shared_ptr<EntityWorld>> worlds;
    GameState state;
    bool show_intro = false;
};

