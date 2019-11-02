#include "pch.hpp"

#include "game_system.hpp"

Game::GameSystemManager::GameSystemManager( Engine::MemoryAllocatorPtr &allocator, GameEntityManagerPtr &entity_manager, GameComponentManagerPtr &component_manager ) :
    m_allocator( allocator ),
    m_component_manager( component_manager ),
    m_entity_manager( entity_manager ),
    m_task_chain_dirty( false )
{
}

void Game::GameSystemManager::RunSystems( float dt )
{
    if( m_task_chain_dirty )
    {
        m_run_order.clear();
        RebuildTaskChain();
        m_task_chain_dirty = false;
    }

    for( auto it : m_run_order )
    {
        it->system->PreUpdate( dt );
        it->system->Update( dt );
        it->system->PostUpdate( dt );
    }
}

void Game::GameSystemManager::RebuildTaskChain()
{
    std::vector<GameSystemEntry*> white;
    white.reserve( m_systems.size() );
    for( auto &entry : m_systems )
    {
        white.push_back( &entry.second );
    }

    std::vector<GameSystemEntry*> grey;
    grey.reserve( m_systems.size() );

    std::vector<GameSystemEntry*> black;
    black.reserve( m_systems.size() );

    std::function<void( GameSystemEntry* )> visit_node;
    visit_node = [&]( GameSystemEntry *visit )
    {
        auto search = std::find_if( white.begin(), white.end(), [&visit]( const GameSystemEntry *entry ) { return entry == visit; } );
        if( search == white.end() )
        {
            if( std::find_if( grey.begin(), grey.end(), [&visit]( const GameSystemEntry *entry ) { return entry == visit; } ) != grey.end() )
            {
                Engine::Log( Engine::LOG_LEVEL_ERROR, L"Cyclic dependency detected in GameSystemManager while trying to build task graph!" );
                assert( false );
            }

            // already visited this node
            return;
        }

        white.erase( search );
        grey.push_back( visit );

        // visit dependencies
        for( auto dependency : visit->dependencies )
        {
            auto search = m_systems.find( dependency );
            if( search != m_systems.end() )
            {
                visit_node( &search->second );
            }
        }

        auto visit_complete = std::find_if( grey.begin(), grey.end(), [&visit]( const GameSystemEntry *entry ) { return entry == visit; } );
        grey.erase( visit_complete );
        black.push_back( visit );
    };

    while( white.size() )
    {
        auto new_visit = white.back();
        white.pop_back();
        visit_node( new_visit );
    }

    for( auto it : black )
    {
        m_run_order.push_back( it );
    }

}


