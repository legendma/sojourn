#include "pch.hpp"

#include "network_message.hpp"

class TestMessage : public Engine::NetworkMessage
{
    friend class Engine::NetworkMessageFactory;
public:

    SERIALIZE_MAPPING()
    {
        stream->Write( a );
        stream->Write( b );
        stream->Write( c );
    }

private:
    int a;
    int b;
    int c;

    TestMessage() : NetworkMessage( Engine::MESSAGE_TEST ) { a = 0; b = 0; c = 0; }
};

Engine::NetworkMessagePtr Engine::NetworkMessageFactory::CreateMessage( NetworkMessageTypeId id )
{
    switch( id )
    {
        case MESSAGE_TEST:
            return NetworkMessagePtr( new TestMessage() );
            break;
    }

    return nullptr;
}
