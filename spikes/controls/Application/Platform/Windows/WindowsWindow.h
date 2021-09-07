#pragma once
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"
#include "Window.h"
#include "PlatformFactory.h"

class WindowsWindow final : public Window
{
public:
    WindowsWindow( HINSTANCE instance, uint16_t width, uint16_t height/*, Keyboard &keyboard, Mouse &mouse, Graphics &graphics*/ );
    HWND GetHwnd() { return hwnd; }

    virtual bool ProcessMessages() override;

private:
    HWND hwnd = nullptr;
    HINSTANCE hinstance = nullptr;
    HACCEL hAccelTable = nullptr;
    std::wstring window_title = L"";
    std::wstring window_class = L"";
    //Keyboard &keyboard;
    //Mouse &mouse;
    //Graphics &graphics;
    std::vector<uint8_t> raw_buffer;

protected:
    void RegisterWindowClass();
    void RegisterRawDevices();
    static LRESULT CALLBACK _WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
    LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
};


