#include "pch.hpp"

#include "game_entity.hpp"

class TestEntityType : public Game::GameEntity<TestEntityType, Game::TEST_ENTITY_TYPE>
{

};

void test()
{
    Game::GameEntityManagerPtr manager;

    auto entity = manager->CreateEntity<TestEntityType>();
}

Game::GameEntityManager::GameEntityManager( Engine::MemoryAllocatorPtr &allocator ) :
    m_allocator( allocator )
{
    auto container = GetEntityContainer<TestEntityType>();
}
