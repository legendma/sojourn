#include "pch.h"
#include "Mouse.h"

Mouse::Mouse()
{
}

void Mouse::OnLeftPressed( int x, int y )
{
    left_down = true;
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::LEFT_PRESSED, x, y ) );
}

void Mouse::OnLeftReleased( int x, int y )
{
    left_down = false;
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::LEFT_RELEASED, x, y ) );
}

void Mouse::OnRightPressed( int x, int y )
{
    right_down = true;
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::RIGHT_PRESSED, x, y ) );
}

void Mouse::OnRightReleased( int x, int y )
{
    right_down = false;
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::RIGHT_RELEASED, x, y ) );
}

void Mouse::OnMiddlePressed( int x, int y )
{
    middle_down = true;
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::MIDDLE_PRESSED, x, y ) );
}

void Mouse::OnMiddleReleased( int x, int y )
{
    middle_down = false;
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::MIDDLE_RELEASED, x, y ) );
}

void Mouse::OnWheelUp( int x, int y )
{
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::WHEEL_UP, x, y ) );
}

void Mouse::OnWheelDown( int x, int y )
{
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::WHEEL_DOWN, x, y ) );
}

void Mouse::OnMove( int x, int y )
{
    position = MouseEvent::MousePoint( x, y );
    events.push( MouseEvent( MouseEvent::MouseEventType::MOVE, x, y ) );
}

void Mouse::OnRawMove( int x, int y )
{
    events.push( MouseEvent( MouseEvent::MouseEventType::RAW_MOVE, x, y ) );
}

MouseEvent Mouse::ReadEvent()
{
    if( events.empty() )
    {
        return MouseEvent();
    }

    auto evt = events.front();
    events.pop();
    return evt;
}
