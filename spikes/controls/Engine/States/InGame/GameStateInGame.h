#pragma once
#include "GameStates.h"
#include "IEngine.h"

class GameStateInGame : public GameStateBase
{
public:
    GameStateInGame( IEngine &engine );
};

