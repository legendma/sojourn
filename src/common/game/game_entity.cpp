#include "pch.hpp"

#include "game_entity.hpp"

Game::GameEntityManager::GameEntityManager( Engine::MemoryAllocatorPtr &allocator, GameComponentManagerPtr &component_manager ) :
    m_allocator( allocator ),
    m_component_manager( component_manager )
{
}

Game::GameEntityId Game::GameEntityManager::GetNewUID( IGameEntity *object )
{
    return m_uids.GetNewUID( object );
}

void Game::GameEntityManager::FreeGarbage()
{
    for( auto entity : m_garbage )
    {
        auto id = entity->GetEntityId();
        m_component_manager->RemoveAllComponents( id );

        auto it = m_entity_containers.find( entity->GetType() );
        assert( it != m_entity_containers.end() );
        it->second->DestroyEntity( entity );

        m_uids.ReleaseUID( id );
    }

    m_garbage.clear();
}

void Game::GameEntityManager::DestroyEntity( GameEntityId entity_id )
{
    auto entity = m_uids.GetObjectByUID( entity_id );
    m_garbage.push_back( entity );
}
