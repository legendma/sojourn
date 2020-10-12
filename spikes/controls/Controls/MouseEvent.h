#pragma once
class MouseEvent
{
public:
    struct MousePoint
    {
        int x;
        int y;

        MousePoint( int x, int y ) : x( x ), y( y ) {}
    };

    enum class MouseEventType
    {
        INVALID,
        LEFT_PRESSED,
        LEFT_RELEASED,
        RIGHT_PRESSED,
        RIGHT_RELEASED,
        MIDDLE_PRESSED,
        MIDDLE_RELEASED,
        WHEEL_UP,
        WHEEL_DOWN,
        MOVE,
        RAW_MOVE
    };

    MouseEvent();
    MouseEvent( const MouseEventType event_type, const int x = 0, const int y = 0 );

    MousePoint GetPos() { return MousePoint( x, y ); }
    MouseEventType GetEvent() { return event_type; }
    int GetX() { return x; }
    int GetY() { return y; }
    bool IsLeftPressed() { return event_type == MouseEventType::LEFT_PRESSED; }
    bool IsLeftReleased() { return event_type == MouseEventType::LEFT_RELEASED; }
    bool IsRightPressed() { return event_type == MouseEventType::RIGHT_PRESSED; }
    bool IsRightReleased() { return event_type == MouseEventType::RIGHT_RELEASED; }
    bool IsMiddlePressed() { return event_type == MouseEventType::MIDDLE_PRESSED; }
    bool IsMiddleReleased() { return event_type == MouseEventType::MIDDLE_RELEASED; }
    bool IsWheelUp() { return event_type == MouseEventType::WHEEL_UP; }
    bool IsWheelDown() { return event_type == MouseEventType::WHEEL_DOWN; }
    bool IsMove() { return event_type == MouseEventType::MOVE; }
    bool IsRawMove() { return event_type == MouseEventType::RAW_MOVE; }
    bool IsValid() { return event_type != MouseEventType::INVALID; }

protected:
    int x = 0;
    int y = 0;
    MouseEventType event_type = MouseEventType::INVALID;
};

