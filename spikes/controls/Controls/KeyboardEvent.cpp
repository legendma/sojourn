#include "pch.h"

#include "KeyboardEvent.h"

KeyboardEvent::KeyboardEvent()
{
}

KeyboardEvent::KeyboardEvent( const KeyboardEventType event_type, const uint8_t key ) :
    event_type( event_type ),
    key( key )
{
}
