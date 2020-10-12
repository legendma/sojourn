#pragma once
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"

class Window
{
public:
    Window( HINSTANCE instance, uint16_t width, uint16_t height, Keyboard &keyboard, Mouse &mouse, Graphics &graphics );
    bool ProcessMessages();

    uint16_t GetWidth() { return window_width; }
    uint16_t GetHeight() { return window_height; }
    HWND GetHwnd() { return hwnd; }

private:
    HWND hwnd = nullptr;
    HINSTANCE hinstance = nullptr;
    HACCEL hAccelTable = nullptr;
    std::wstring window_title = L"";
    std::wstring window_class = L"";
    uint16_t window_width = 0;
    uint16_t window_height = 0;
    Keyboard &keyboard;
    Mouse &mouse;
    Graphics &graphics;
    std::vector<uint8_t> raw_buffer;

protected:
    void RegisterWindowClass();
    void RegisterRawDevices();
    static LRESULT CALLBACK _WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
    LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
};

