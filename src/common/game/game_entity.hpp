#pragma once

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
        GameEntity()
        {
        }

        virtual GameEntityType GetType() { return m_entity_type; }

        static const GameEntityType m_entity_type = _EntityType;
    };

    class GameEntityManager
    {
    public:
        template <typename T>
        GameEntityId CreateEntity()
        {
            auto entity = new T();
            GameEntityType entity_type = T::m_entity_type;
            return 0;
        }
    };
}
