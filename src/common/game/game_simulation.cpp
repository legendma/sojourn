#include "pch.hpp"

#include "game_simulation.hpp"
#include "game_player.hpp"

#define GAME_ECS_MEMORY_SIZE ( 128 * 1024 * 1024 )

Game::GameSimulation::GameSimulation()
{
    m_ecs_memory = Engine::MemoryAllocatorPtr( new Engine::MemorySystem( GAME_ECS_MEMORY_SIZE ) );

    m_component_manager = GameComponentManagerPtr( new GameComponentManager( m_ecs_memory ) );
    m_entity_manager = GameEntityManagerPtr( new GameEntityManager( m_ecs_memory, m_component_manager ) );
}

void Game::GameSimulation::RunFrame( float dt )
{
    // process all the players' network messages
    for( auto player : m_players )
    {
        auto message = player.channel->PopIncomingMessage();
        while( message )
        {
            message = player.channel->PopIncomingMessage();
            ProcessPlayerMessage( player.player_id, message );
        }
    }

    // TODO <MPA>: Run the systems
}

void Game::GameSimulation::AddPlayer( Engine::NetworkReliableEndpointPtr &channel )
{
    m_players.emplace_back();
    auto &player = m_players.back();

    player.player_id = m_entity_manager->CreateEntity<GamePlayer>();
    player.channel = channel;
}

void Game::GameSimulation::ProcessPlayerMessage( GameEntityId player_id, Engine::NetworkMessagePtr &message )
{
}
