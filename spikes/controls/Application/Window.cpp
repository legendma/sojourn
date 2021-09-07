#include "pch.h"
#include "Window.h"

Window::Window( int width, int height ) :
    window_width( width ),
    window_height( height )
{
}

bool Window::ProcessMessages()
{
    return false;
}
