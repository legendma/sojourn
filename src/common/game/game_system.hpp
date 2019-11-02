#pragma once

#include "game_component.hpp"
#include "game_entity.hpp"

namespace Game
{
    typedef enum
    {
        RENDER_SYSTEM,
        PREGAME_SYSTEM,
        SYSTEM_TYPE_CNT
    } GameSystemType;

    class IGameSystem
    {
        friend class GameSystemManager;
    public:
        virtual ~IGameSystem() {};
        virtual GameSystemType GetType() = 0;
        virtual void PreUpdate( float delta_time ) {};
        virtual void Update( float delta_time ) {};
        virtual void PostUpdate( float delta_time ) {};

    protected:
        GameComponentManager *m_component_manager;
        GameEntityManager *m_entity_manager;
        class GameSystemManager *m_system_manager;
    };

    template <typename T, GameSystemType _SystemType>
    class GameSystem : public IGameSystem
    {
    public:
        GameSystem() = default;

        virtual GameSystemType GetType() { return SYSTEM_TYPE; }

        static const GameSystemType SYSTEM_TYPE = _SystemType;
    };

    class GameSystemManager
    {
        struct GameSystemEntry
        {
            IGameSystem *system;
            std::list<GameSystemType> dependencies;
        };

    public:
        GameSystemManager( Engine::MemoryAllocatorPtr &allocator, GameEntityManagerPtr &entity_manager, GameComponentManagerPtr &component_manager );
        void RunSystems( float dt );

        template <typename T, typename ...ARGS>
        T * CreateSystem( ARGS... args )
        {
            auto search = m_systems.find( T::SYSTEM_TYPE );
            if( search != m_systems.end() )
            {
                assert( search->second.system != nullptr );
                return reinterpret_cast<T*>( search->second.system );
            }

            m_systems.emplace( std::make_pair( T::SYSTEM_TYPE, GameSystemEntry() ) );
            auto &entry = m_systems[ T::SYSTEM_TYPE ];

            auto memory = m_allocator->Allocate( sizeof( T ), L"GameSystemManager" );
            reinterpret_cast<IGameSystem*>( memory )->m_entity_manager = &*m_entity_manager;
            reinterpret_cast<IGameSystem*>( memory )->m_component_manager = &*m_component_manager;

            entry.system = new(memory) T( std::forward<ARGS>( args )... );
            entry.dependencies.resize( SYSTEM_TYPE_CNT );

            m_task_chain_dirty = true;

            return reinterpret_cast<T*>( entry.system );
        }

        template <typename T>
        inline T * GetSystem()
        {
            auto search = m_systems.find( T::SYSTEM_TYPE );
            if( search == m_systems.end() )
            {
                return nullptr;
            }

            return reinterpret_cast<T*>( search->second.system );
        }

    private:
        Engine::MemoryAllocatorPtr m_allocator;
        GameComponentManagerPtr m_component_manager;
        GameEntityManagerPtr m_entity_manager;
        std::unordered_map<GameSystemType, GameSystemEntry> m_systems;
        bool m_task_chain_dirty;
        std::vector<GameSystemEntry*> m_run_order;

        void RebuildTaskChain();

    }; typedef std::shared_ptr<GameSystemManager> GameSystemManagerPtr;
}