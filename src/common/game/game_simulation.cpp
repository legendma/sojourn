#include "pch.hpp"

#include "game_simulation.hpp"

#define GAME_ECS_MEMORY_SIZE ( 128 * 1024 * 1024 )

Game::GameSimulation::GameSimulation()
{
    m_ecs_memory = Engine::MemoryAllocatorPtr( new Engine::MemorySystem( GAME_ECS_MEMORY_SIZE ) );
    m_entity_manager = GameEntityManagerPtr( new GameEntityManager( m_ecs_memory ) );
}
