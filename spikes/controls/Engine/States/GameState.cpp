#include "pch.h"
#include "GameState.h"
#include "GameStateIntro.h"
#include "GameStateMainMenu.h"
#include "GameStateInGame.h"

void GameState::ChangeState( const GameStateType new_state, IEngine &engine )
{
    if( current_state
     && current_state->state_name == new_state )
    {
        // no change
        DebugAssert( false );
        return;
    }

    if( current_state )
    {
        current_state->OnExitState();
        current_state.reset();
    }

    switch( new_state )
    {
        case GameStateType::GAME_STATE_INTRO:
            current_state = std::make_unique<GameStateIntro>( engine );
            break;
            
        case GameStateType::GAME_STATE_MAIN_MENU:
            current_state = std::make_unique<GameStateMainMenu>( engine );
            break;

        case GameStateType::GAME_STATE_IN_GAME:
            current_state = std::make_unique<GameStateInGame>( engine );
            break;
    }

    current_state->OnEnterState();
        
}

void GameState::Tick( const float millisec )
{
    if( !current_state )
    {
        return;
    }

    current_state->Tick( millisec );
}
