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

Engine::NetworkMessagePtr Engine::NetworkMessageFactory::CreateMessage( InputBitStreamPtr &read )
{
    auto marker = read->SaveCurrentLocation();
    NetworkMessageTypeId message_type;
    read->Write( message_type, read->BitsRequired( Engine::MESSAGE_TYPE_CNT ) );
    read->SeekToLocation( marker );

    auto message = Engine::NetworkMessageFactory::CreateMessage( message_type );
    message->Serialize( read );

    return message;
}
