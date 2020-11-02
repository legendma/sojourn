#pragma once

using TypeId = uint32_t;
struct TypeSequence
{
    static TypeId NextSequenceId()
    {
        static TypeId next_id = {};
        return next_id++;
    }
};

template<typename T>
TypeId GetTypeId()
{
    static TypeId id = TypeSequence::NextSequenceId();
    return id;
}