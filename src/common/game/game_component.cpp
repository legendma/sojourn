#include "pch.hpp"

#include "game_system.hpp"
#include "game_component.hpp"

Game::GameComponentManager::GameComponentManager( Engine::MemoryAllocatorPtr &allocator ) :
    m_allocator( allocator )
{
}

void Game::GameComponentManager::RemoveAllComponents( GameEntityId entity_id )
{
    assert( m_entity_components.size() >= entity_id.m_index + 1 );
    auto &components = m_entity_components[ entity_id.m_index ];
    for( int component_type = 0; component_type < COMPONENT_TYPE_CNT; component_type++ )
    {
        auto &component = components[ component_type ];
        if( component == nullptr )
        {
            continue;
        }

        auto container = m_component_containers.find( static_cast<GameComponentType>( component_type ) );
        container->second->DestroyComponent( components[ component_type ] );
        component = nullptr;
    }
}

void Game::GameComponentManager::AddEntityComponentMap( GameEntityId entity_id, GameComponentType component_type, IGameComponent *new_component )
{
    auto old_size = m_entity_components.size();
    if( old_size <= entity_id.m_index )
    {
        auto new_size = old_size + GAME_UID_GROW_SIZE;
        m_entity_components.resize( new_size );
        for( auto i = old_size; i < new_size; i++ )
        {
            m_entity_components[ entity_id.m_index ].resize( COMPONENT_TYPE_CNT, nullptr );
        }
    }

    assert( m_entity_components[ entity_id.m_index ][ component_type ] == nullptr );
    m_entity_components[ entity_id.m_index ][ component_type ] = new_component;
}
