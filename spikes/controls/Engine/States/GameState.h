#pragma once
#include "GameStates.h"
#include "IEngine.h"

class GameState
{
public:
    GameState() = default;

    void ChangeState( const GameStateType new_state, IEngine &engine );

private:
    std::unique_ptr<GameStateBase> current_state;
};

