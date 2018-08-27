#include "pch.hpp"

#include "game_entity.hpp"

class TestEntityType : public Game::GameEntity<TestEntityType, Game::TEST_ENTITY_TYPE>
{

};

void test()
{
    Game::GameEntityManager manager;

    auto entity = manager.CreateEntity<TestEntityType>();
}