#pragma once

#include "Entity.h"
#include "ComponentView.h"
#include "ComponentPool.h"

class EntityWorld
{
private:
    struct PoolRecord
    {
        //std::unique_ptr<ComponentPool>
    };

public:
    EntityWorld() = default;

    template<typename... Component>
    ComponentView<Component...> GetTuple();

    //template<typename Component>
    //ComponentPool<Component> & GetComponentPool();

private:
    Entity singleton;
    std::vector<PoolRecord> pools;
};

template<typename ...Component>
inline ComponentView<Component...> EntityWorld::GetTuple()
{
    return ComponentView<Component...>();
}
