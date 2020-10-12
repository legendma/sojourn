#pragma once
#include "MouseEvent.h"

class Mouse
{
public:
    Mouse();

    void OnLeftPressed( int x, int y );
    void OnLeftReleased( int x, int y );
    void OnRightPressed( int x, int y );
    void OnRightReleased( int x, int y );
    void OnMiddlePressed( int x, int y );
    void OnMiddleReleased( int x, int y );
    void OnWheelUp( int x, int y );
    void OnWheelDown( int x, int y );
    void OnMove( int x, int y );
    void OnRawMove( int x, int y );

    MouseEvent ReadEvent();
    bool IsEventBufferEmpty() { return events.empty(); }

    MouseEvent::MousePoint GetPosition() { return position; }
    bool IsLeftDown() { return left_down; }
    bool IsRightDown() { return right_down; }
    bool IsMiddleDown() { return middle_down; }

private:
    MouseEvent::MousePoint position = { 0, 0 };
    bool left_down = false;
    bool right_down = false;
    bool middle_down = false;
    std::queue<MouseEvent> events;
};

