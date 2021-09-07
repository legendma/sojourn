// controls.cpp : Defines the entry point for the application.
//
#include "pch.h"

#include "Main.h"
#include "Engine.h"
#include "Platform.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Engine engine( hInstance );
    Platform platform( engine );

    engine.Run();
    
    return 0;
}