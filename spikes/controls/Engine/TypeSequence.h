#pragma once

using TypeId = size_t;
struct PlatformTypeSequence
{
    static TypeId NextSequenceId()
    {
        static TypeId next_id = {};
        return next_id++;
    }

    template<typename T>
    static TypeId Id()
    {
        static TypeId id = PlatformTypeSequence::NextSequenceId();
        return id;
    }
};