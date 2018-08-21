#pragma once

#include "network_buffers.hpp"

#define SERIALIZE_MAPPING()                                        \
    virtual void Serialize( Engine::MeasureBitStreamPtr &measure ) \
    {                                                              \
        __Serialize( measure );                                    \
    }                                                              \
                                                                   \
    virtual void Serialize( Engine::InputBitStreamPtr &read )      \
    {                                                              \
        __Serialize( read );                                       \
    }                                                              \
                                                                   \
    virtual void Serialize( Engine::OutputBitStreamPtr &write )    \
    {                                                              \
        __Serialize( write );                                      \
    }                                                              \
                                                                   \
    template <typename T>                                          \
    void __Serialize( T &stream )

namespace Engine
{
    typedef enum
    {
        MESSAGE_TEST
    } NetworkMessageTypeId;

    class NetworkMessage
    {
    public:
        virtual void Serialize( MeasureBitStreamPtr &measure ) = 0;
        virtual void Serialize( InputBitStreamPtr &read ) = 0;
        virtual void Serialize( OutputBitStreamPtr &write ) = 0;

        NetworkMessageTypeId message_type;

    protected:
        NetworkMessage( NetworkMessageTypeId id ) : message_type( id ) {};
    }; typedef std::shared_ptr<NetworkMessage> NetworkMessagePtr;

    class NetworkMessageFactory
    {
    public:
        static NetworkMessagePtr CreateMessage( NetworkMessageTypeId id );
    };
}

