#include "pch.hpp"

#include "game_player.hpp"

class TestComponent : public Game::GameComponent<TestComponent, Game::TEST_COMPONENT>
{
public:
TestComponent( std::string component_name ) { m_myname = component_name; DoSomeInit(); }

protected:
    void DoSomeInit()
    {

    }
private:
    std::string m_myname;

};

class AnotherTestComponent : public Game::GameComponent<AnotherTestComponent, Game::ANOTHER_TEST_COMPONENT>
{

};

Game::GamePlayer::GamePlayer()
{
    // testing REMOVE <MPA>
    auto test = m_component_manager->begin<TestComponent>() == m_component_manager->end<TestComponent>();
    for( auto it = m_component_manager->begin<TestComponent>(); it != m_component_manager->end<TestComponent>(); it++ )
    {
        m_component_manager->AddComponent<TestComponent>( m_entity_id, std::string( "Illustrious component name" ) );
    }

    // @INCOMPLETE - <MPA> Make this work
    //auto tupled = m_component_manager->begin<TestComponent, AnotherTestComponent>();
}