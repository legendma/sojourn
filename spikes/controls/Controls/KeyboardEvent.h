#pragma once

class KeyboardEvent
{
public:
    enum class KeyboardEventType
    {
        INVALID,
        PRESSED,
        RELEASED
    };

    KeyboardEvent();
    KeyboardEvent( const KeyboardEventType event_type, const uint8_t key );

    uint8_t GetKey() { return key; }
    KeyboardEventType GetEvent() { return event_type; }
    bool IsPressed() { return event_type == KeyboardEventType::PRESSED; }
    bool IsReleased() { return event_type == KeyboardEventType::RELEASED; }
    bool IsValid() { return event_type != KeyboardEventType::INVALID; }

private:
    KeyboardEventType event_type = KeyboardEventType::INVALID;
    uint8_t key = 0;
};

