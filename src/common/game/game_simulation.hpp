#pragma once

#include "common/engine/engine_memory.hpp"
#include "common/network/network_message.hpp"
#include "common/network/network_reliable_endpoint.hpp"
#include "game_entity.hpp"

namespace Game
{
    class GameSimulation
    {
    public:
        GameSimulation();
        void RunFrame( float dt );
        void AddPlayer( Engine::NetworkReliableEndpointPtr &channel );

    private:
        struct PlayerEntry
        {
            Engine::NetworkReliableEndpointPtr channel;
            GameEntityId player_id;
        };

        Engine::MemoryAllocatorPtr m_ecs_memory;
        GameComponentManagerPtr m_component_manager;
        GameEntityManagerPtr m_entity_manager;
        std::vector<PlayerEntry> m_players;

        void ProcessPlayerMessage( GameEntityId player_id, Engine::NetworkMessagePtr &message );

    }; typedef std::shared_ptr<GameSimulation> GameSimulationPtr;
}