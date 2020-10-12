#include "pch.h"
#include "MouseEvent.h"

MouseEvent::MouseEvent()
{
}

MouseEvent::MouseEvent( const MouseEventType event_type, const int x, const int y ) :
    event_type( event_type ),
    x( x ),
    y( y )
{
}
