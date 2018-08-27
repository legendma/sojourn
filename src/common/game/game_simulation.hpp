#pragma once

#include "common/engine/engine_memory.hpp"
#include "game_entity.hpp"

namespace Game
{
    class GameSimulation
    {
    public:
        GameSimulation();

    private:
        Engine::MemoryAllocatorPtr m_ecs_memory;
        GameEntityManagerPtr m_entity_manager;

    }; typedef std::shared_ptr<GameSimulation> GameSimulationPtr;
}