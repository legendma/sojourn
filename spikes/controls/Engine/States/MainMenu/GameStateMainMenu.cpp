#include "pch.h"
#include "GameStateMainMenu.h"

GameStateMainMenu::GameStateMainMenu( IEngine &engine ) :
    GameStateBase( GameStateType::GAME_STATE_MAIN_MENU, engine )
{
}

void GameStateMainMenu::OnEnterState()
{
    engine.AddWorld( &world );
    gui_target = graphics.CreateRenderTarget( graphics.GetBackBufferWidth(), graphics.GetBackBufferHeight() );
}

void GameStateMainMenu::OnExitState()
{
    engine.RemoveWorld( &world );
}

void GameStateMainMenu::Update( float millisec )
{
}

void GameStateMainMenu::Render( float millisec )
{
    graphics.Clear( 0.0f, 0.0f, 0.0f, 1.0f );
    gui_target->Set();
    graphics.Present();
}

//void GameStateMainMenu::Tick( float timestep )
//{
//}
