#pragma once

enum class Component : size_t
{
    MY_COMPONENT,
    ANOTHER_COMPONENT
};

template <size_t Id>
struct ComponentTraits
{
    static constexpr size_t id = Id;
};

template<typename T>
constexpr auto As( T value )
{
    return static_cast<std::underlying_type_t<T>>( value );
}

struct HairyComponent : ComponentTraits<As( Component::MY_COMPONENT )>
{

};

struct PoolType {};

template <typename T>
class ConcretePool : PoolType
{
public:
    std::vector<T> allocs;
};

class Test
{
public:
    template <typename T>
    size_t TestMe()
    {
    return T::id;
    }

    std::vector<PoolType> pools;

    void Now()
    {
        auto id = TestMe<HairyComponent>();

        auto &the_pool = GetPool<HairyComponent>();
    }

    template <typename T>
    PoolType & GetPool()
    {
        auto index = T::id;
        return pools[ index ];
    }
};