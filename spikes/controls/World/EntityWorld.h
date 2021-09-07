#pragma once

#include "Entity.h"
#include "ComponentView.h"
#include "ComponentPool.h"
#include "SparseSet.h"

class EntityWorld
{
private:
    struct Pool
    {
        std::unique_ptr<SparseSet<Entity>> pool;
    };

public:
    EntityWorld() = default;

    template<typename... Component>
    ComponentView<Component...> GetTuple()
    {

    }

    template<typename ComponentType>
    Pool & GetComponentPool()
    {
        size_t id = ComponentType::id;
        if( !(id < pools.size() ) )
        {
            pools.resize( id + 1 );
        }

        if( !pools[ id ].pool )
        {
            pools[ id ].pool =  std::make_unique<ComponentPool<ComponentType>>();
        }
        
        return static_cast<SparseSet<Entity>>( pools[ id ].pool );
    }

private:
    Entity singleton;
    std::vector<Pool> pools;
};