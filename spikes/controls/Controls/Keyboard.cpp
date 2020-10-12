#include "pch.h"
#include "Keyboard.h"

KeyboardEvent Keyboard::ReadKeyEvent()
{
    if( events.empty() )
    {
        return KeyboardEvent();
    }
    auto &evt = events.front();
    events.pop();
    return evt;
}

uint8_t Keyboard::ReadChar()
{
    if( characters.empty() )
    {
        return 0;
    }

    auto character = characters.front();
    characters.pop();
    return character;
}

void Keyboard::OnPressed( uint8_t key )
{
    events.push( KeyboardEvent( KeyboardEvent::KeyboardEventType::PRESSED, key ) );
    key_states.set( key );
}

void Keyboard::OnReleased( uint8_t key )
{
    events.push( KeyboardEvent( KeyboardEvent::KeyboardEventType::RELEASED, key ) );
    key_states.reset( key );

}
