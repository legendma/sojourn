#include "pch.h"
#include "Platform.h"
#include "WindowsWindow.h"

Platform::Platform( Engine &engine )
{
    HINSTANCE hinstance;
    engine.RegisterPlatform<Window>( [hinstance]( int width, int height )
    {
        return std::make_unique<WindowsWindow>( hinstance, width, height );
    } );


}
