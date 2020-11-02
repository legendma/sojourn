#pragma once
#include "KeyboardEvent.h"

class Keyboard
{
public:
    KeyboardEvent ReadKeyEvent();
    uint8_t ReadChar();

    void OnChar( uint8_t key ) { characters.push( key ); }
    void OnPressed( uint8_t key );
    void OnReleased( uint8_t key );

    bool IsEventBufferEmpty() { return events.empty(); }
    bool IsCharBufferEmpty() { return characters.empty(); }
    bool IsCharAutoRepeat() { return is_auto_repeat_chars; }
    void SetCharAutoRepeat( bool set ) { is_auto_repeat_chars = set; }
    bool IsEventAutoRepeat() { return is_auto_repeat_events; }
    void SetEventAutoRepeat( bool set ) { is_auto_repeat_events = set; }
    bool IsKeyPressed( uint8_t key ) { return key_states.test( key ); }

private:
    std::bitset<256> key_states;
    std::queue<KeyboardEvent> events;
    std::queue<uint8_t> characters;
    bool is_auto_repeat_chars = false;
    bool is_auto_repeat_events = false;
};