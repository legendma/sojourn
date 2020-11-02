#pragma once
#include "GameStates.h"
#include "IEngine.h"
#include "EntityWorld.h"

class GameStateMainMenu : public GameStateBase
{
public:
    GameStateMainMenu( IEngine &engine );

    virtual void OnEnterState() override;
    virtual void OnExitState() override;
    //virtual void Tick( float timestep ) override;
    virtual void Update( float millisec ) override;
    virtual void Render( float millisec ) override;

private:
    EntityWorld world;
    std::shared_ptr<RenderTarget> gui_target;
};

