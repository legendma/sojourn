#pragma once

#include "common/engine/engine_memory.hpp"

#include "game_component.hpp"
#include "game_main.hpp"

#define GAME_ENTITIES_PER_CHUNK ( 512 )

namespace Game
{
    typedef enum
    {
        PLAYER_ENTITY,
        ENTITY_TYPE_CNT
    } GameEntityType;

    class IGameEntity
    {
        friend class GameEntityManager;
    public:
        virtual ~IGameEntity() {};
        virtual GameEntityType GetType() = 0;

        inline const GameEntityId GetEntityId() { return m_entity_id; }

    protected:
        GameEntityId m_entity_id;
        GameComponentManager *m_component_manager;
    };

    template <typename T, GameEntityType _EntityType>
    class GameEntity : public IGameEntity
    {
    public:
        GameEntity() {};

        virtual GameEntityType GetType() { return ENTITY_TYPE; }

        static const GameEntityType ENTITY_TYPE = _EntityType;
    };

    class GameEntityManager
    {
    private:
        typedef std::shared_ptr<IGameEntity> GameEntityPtr;

        class GameEntityContainerBase
        {   
        public:
            virtual void DestroyEntity( IGameEntity *entity ) = 0;
        }; typedef std::shared_ptr<GameEntityContainerBase> GameEntityContainerPtr;

        template <typename T>
        class GameEntityContainer : public Engine::MemoryChunkAllocator<T, GAME_ENTITIES_PER_CHUNK>,
                                    public GameEntityContainerBase
        {
        public:
            GameEntityContainer( Engine::MemoryAllocatorPtr &allocator ) :
                Engine::MemoryChunkAllocator<T, GAME_ENTITIES_PER_CHUNK>( allocator, L"GameEntityContainer" )
            {}

            virtual void DestroyEntity( IGameEntity *entity )
            {
                entity->~IGameEntity();
                this->Free( entity );
            }
        };

    public:
        GameEntityManager( Engine::MemoryAllocatorPtr &allocator, GameComponentManagerPtr &component_manager );
        GameEntityId GetNewUID( IGameEntity *object );
        void FreeGarbage();
        void DestroyEntity( GameEntityId entity_id );

        template <typename T, typename ...ARGS>
        GameEntityId CreateEntity( ARGS... args )
        {
            auto container = GetEntityContainer<T>();
            auto memory = container.Allocate();
            auto entity_id = GetNewUID( reinterpret_cast<IGameEntity*>( memory ) );

            reinterpret_cast<IGameEntity*>( memory )->m_entity_id = entity_id;
            reinterpret_cast<IGameEntity*>( memory )->m_component_manager = &*m_component_manager;
            new(memory) GameEntity<T, T::ENTITY_TYPE>( std::forward<ARGS>( args )... );

            return entity_id;
        }

        inline IGameEntity * GetEntityByID( GameEntityId entity_id )
        {
            return m_uids.GetObjectByUID( entity_id );
        }

    private:
        Engine::MemoryAllocatorPtr m_allocator;
        std::unordered_map<GameEntityType, GameEntityContainerPtr> m_entity_containers;
        GameComponentManagerPtr m_component_manager;
        GameUIDTable<IGameEntity, GameEntityId> m_uids;
        std::vector<IGameEntity*> m_garbage;

        template <typename T>
        GameEntityContainer<T> & GetEntityContainer()
        {
            auto found = m_entity_containers.find( T::ENTITY_TYPE );
            if( found != m_entity_containers.end() )
            {
                return reinterpret_cast<GameEntityContainer<T>&>(*found->second);
            }

            auto new_container = GameEntityContainerPtr( new GameEntityContainer<T>( m_allocator ) );
            m_entity_containers[T::ENTITY_TYPE] = new_container;

            return reinterpret_cast<GameEntityContainer<T>&>(*new_container);
        }

    }; typedef std::shared_ptr<GameEntityManager> GameEntityManagerPtr;

}
