#include "pch.hpp"

#include "game_player.hpp"

class TestComponent : public Game::GameComponent<TestComponent, Game::PLAYER_COMPONENT>
{

};

Game::GamePlayer::GamePlayer()
{
    // testing REMOVE <MPA>
    auto test = m_component_manager->begin<TestComponent>() == m_component_manager->end<TestComponent>();
    for( auto it = m_component_manager->begin<TestComponent>(); it != m_component_manager->end<TestComponent>(); it++ )
    {
        m_component_manager->AddComponent<TestComponent>( m_entity_id );
    }
}