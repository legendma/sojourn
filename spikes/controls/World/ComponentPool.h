#pragma once

#include "SparseSet.h"

template <typename EntityType, typename ComponentType>
class ComponentPool : public SparseSet<EntityType>
{
    using BaseClass = SparseSet<EntityType>;
public:
    template<typename... Args>
    ComponentType & Add( const EntityType k, Args &&... args )
    {
        if constexpr( std::is_aggregate_v<ComponentType> )
        {
            values.push_back( ComponentType{ std::forward<Args>( args )... } );
        }
        else
        {
            values.emplace_back( std::forward<Args>( args )... );
        }

        BaseClass::Add( k );
        return values.back();
    }

    virtual void Clear() override
    {
        BaseClass::Clear();
        values.clear();
    }

    void Remove( EntityType k )
    {
        auto swap_value = std::move( values.back() );
        values[ base_class::Get( k ) ] = std::move( swap_value );
        BaseClass::Remove( k );
        values.pop_back();
    }

private:
    std::vector<ComponentType> values;
};