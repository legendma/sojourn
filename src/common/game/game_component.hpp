#pragma once

#include "common/engine/engine_memory.hpp"
#include "common/engine/engine_utilities.hpp"

#include "game_main.hpp"

#define GAME_COMPONENTS_PER_CHUNK ( 512 )

namespace Game
{
    typedef enum
    {
        TEST_COMPONENT,
        ANOTHER_TEST_COMPONENT,
        COMPONENT_TYPE_CNT
    } GameComponentType;

    class IGameComponent
    {
        GameEntityId m_owning_entity_id;
    };

    template <typename T, GameComponentType _ComponentType>
    class GameComponent : public IGameComponent
    {
    public:
        GameComponent() {};

        virtual GameComponentType GetType() { return COMPONENT_TYPE; }

        static const GameComponentType COMPONENT_TYPE = _ComponentType;
    };

    class GameComponentManager
    {
        class GameComponentContainerBase
        {
        public:
            virtual void DestroyComponent( IGameComponent *component ) = 0;
        }; typedef std::shared_ptr<GameComponentContainerBase> GameComponentContainerPtr;

        template <typename T>
        class GameComponentContainer : public Engine::MemoryChunkAllocator<T, GAME_COMPONENTS_PER_CHUNK>,
            public GameComponentContainerBase
        {
        public:
            GameComponentContainer( Engine::MemoryAllocatorPtr &allocator ) :
                Engine::MemoryChunkAllocator<T, GAME_COMPONENTS_PER_CHUNK>( allocator, L"GameComponentContainer" )
            {}

            virtual void DestroyComponent( IGameComponent *component )
            {
                component->~IGameComponent();
                this->Free( component );
            }
        };

    public:
        GameComponentManager( Engine::MemoryAllocatorPtr &allocator );
        void RemoveAllComponents( GameEntityId entity_id );

        template <typename T, typename ...ARGS>
        T* AddComponent( GameEntityId entity_id, ARGS... args )
        {
            assert( m_entity_components.size() < entity_id.m_index + 1 || m_entity_components[ entity_id.m_index ][ T::COMPONENT_TYPE ] == nullptr );
            auto container = GetComponentContainer<T>();
            auto memory = container.Allocate();

            T *new_component = new(memory) T( std::forward<ARGS>( args )... );
            AddEntityComponentMap( entity_id, T::COMPONENT_TYPE, new_component );

            return new_component;
        }

        template <typename T>
        void RemoveComponent( GameEntityId entity_id )
        {
            assert( m_entity_components.size() >= entity_id.m_index + 1 );
            auto &components = m_entity_components[ entity_id.m_index ];
            auto &component = components[ T::COMPONENT_TYPE ];
            if( component == nullptr )
            {
                Engine::Log( Engine::LOG_LEVEL_WARNING, L"Trying to remove a game component from an entity, when the component doesn't exist!" );
                return;
            }

            auto container = GetComponentContainer<T>();
            container.DestroyComponent( component );
            component = nullptr;
        }

        template <typename T>
        typename GameComponentContainer<T>::iterator begin()
        {
            return GetComponentContainer<T>().begin();
        }

        template <typename T>
        typename GameComponentContainer<T>::iterator end()
        {
            return GetComponentContainer<T>().end();
        }

    private:
        Engine::MemoryAllocatorPtr m_allocator;
        std::unordered_map<GameComponentType, GameComponentContainerPtr> m_component_containers;
        std::vector<std::vector<IGameComponent*>> m_entity_components;

        template <typename T>
        GameComponentContainer<T> & GetComponentContainer()
        {
            auto found = m_component_containers.find( T::COMPONENT_TYPE );
            if( found != m_component_containers.end() )
            {
                return reinterpret_cast<GameComponentContainer<T>&>(*found->second);
            }

            auto new_container = GameComponentContainerPtr( new GameComponentContainer<T>( m_allocator ) );
            m_component_containers[ T::COMPONENT_TYPE ] = new_container;
            
            return reinterpret_cast<GameComponentContainer<T>&>(*new_container);
        }

        void AddEntityComponentMap( GameEntityId entity_id, GameComponentType component_type, IGameComponent *new_component );

    }; typedef std::shared_ptr<GameComponentManager> GameComponentManagerPtr;
}