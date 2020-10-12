#pragma once
#include "GameStates.h"
#include "IEngine.h"

class GameStateIntro : public GameStateBase
{
public:
    GameStateIntro( IEngine &engine );
};

