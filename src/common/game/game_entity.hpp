#pragma once

#include "common/engine/engine_memory.hpp"

#define GAME_ENTITIES_PER_CHUNK ( 512 )

namespace Game
{
    typedef enum
    {
        TEST_ENTITY_TYPE,
        ENTITY_TYPE_CNT
    } GameEntityType;

    typedef uint64_t GameEntityId;

    class IGameEntity
    {
    public:
        virtual GameEntityType GetType() = 0;

        inline const GameEntityId GetEntityId() { return m_entity_id; }

    protected:
        GameEntityId m_entity_id;
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

        interface IGameEntityContainer
        {
            virtual GameEntityPtr CreateObject() = 0;
            virtual void DestroyObject( GameEntityPtr &entity ) = 0;
        }; typedef std::shared_ptr<IGameEntityContainer> GameEntityContainerPtr;

        template <typename T>
        class GameEntityContainer : public Engine::MemoryChunkAllocator<T, GAME_ENTITIES_PER_CHUNK>,
                                    public IGameEntityContainer
        {
        public:
            GameEntityContainer( Engine::MemoryAllocatorPtr &allocator ) :
                Engine::MemoryChunkAllocator<T, GAME_ENTITIES_PER_CHUNK>( allocator, L"GameEntityContainer" )
            {}

            virtual GameEntityPtr CreateObject()
            {
                return nullptr;
            }

            virtual void DestroyObject( GameEntityPtr &entity )
            {

            }

        private:
            Engine::MemoryAllocatorPtr m_allocator;
        };

    public:
        GameEntityManager( Engine::MemoryAllocatorPtr &allocator );

        template <typename T>
        GameEntityId CreateEntity()
        {
            auto container = GetEntityContainer<T>();
            //auto entity_id = container->CreateEntityInstance();

            //return entity_id;
            return 0;
        }

        template <typename T>
        GameEntityContainerPtr GetEntityContainer()
        {
            auto found = m_entity_containers.find( T::ENTITY_TYPE );
            if( found != m_entity_containers.end() )
            {
                return found->second;
            }

            auto new_container = GameEntityContainerPtr( new GameEntityContainer<T>( m_allocator ) );
            m_entity_containers[ T::ENTITY_TYPE ] = new_container;

            return new_container;
        }

    private:
        Engine::MemoryAllocatorPtr m_allocator;
        std::unordered_map<GameEntityType, GameEntityContainerPtr> m_entity_containers;

    }; typedef std::shared_ptr<GameEntityManager> GameEntityManagerPtr;

}
