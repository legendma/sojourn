#include "pch.h"

#include "Window.h"
#include "Resource.h"
#include "Utils.h"

Window::Window( HINSTANCE instance, uint16_t width, uint16_t height, Keyboard &keyboard, Mouse &mouse, Graphics &graphics ) :
    keyboard( keyboard ),
    mouse( mouse ),
    graphics( graphics )
{
    this->hinstance = instance;
    this->window_title = L"Control Spike";
    this->window_class = L"Control Spike Window";
    this->window_width = width;
    this->window_height = height;

    hAccelTable = LoadAccelerators( hinstance, MAKEINTRESOURCE( IDC_CONTROLS ) );

    RegisterWindowClass();
    hwnd = CreateWindowW( window_class.c_str(),
                          window_title.c_str(),
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          0,
                          window_width,
                          window_height,
                          nullptr,
                          nullptr,
                          hinstance,
                          this );

    if( !hwnd )
    {
        exit( -1 );
    }

    ShowWindow( hwnd, SW_SHOW );
    SetForegroundWindow( hwnd );
    SetFocus( hwnd );

    RegisterRawDevices();
}

bool Window::ProcessMessages()
{    
    MSG msg = { 0 };
    while( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
    {
        if( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        if( msg.message == WM_QUIT )
        {
            return( false );
        }
    }

    return( true );
}

void Window::RegisterWindowClass()
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof( WNDCLASSEX );

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = Window::_WindowProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hinstance;
    wcex.hIcon         = LoadIcon( hinstance, MAKEINTRESOURCE( IDI_CONTROLS ) );
    wcex.hCursor       = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wcex.lpszMenuName  = MAKEINTRESOURCEW( IDC_CONTROLS );
    wcex.lpszClassName = window_class.c_str();
    wcex.hIconSm       = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SMALL ) );

    RegisterClassExW( &wcex );
}

void Window::RegisterRawDevices()
{
    std::vector<RAWINPUTDEVICE> devices;
    RAWINPUTDEVICE rid;

    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = 0; // = RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
    rid.hwndTarget = nullptr;
    devices.push_back( rid );
    
    //rid.usUsagePage = 0x01;
    //rid.usUsage = 0x06;
    //rid.dwFlags = 0; // = RIDEV_NOLEGACY;   // adds HID keyboard and also ignores legacy keyboard messages
    //rid.hwndTarget = nullptr;
    //devices.push_back( rid );

    if( RegisterRawInputDevices( &devices[ 0 ], static_cast<UINT>( devices.size() ), sizeof( rid ) ) == FALSE )
    {
        exit( -1 );
        //registration failed. Call GetLastError for the cause of the error
    }
}

LRESULT Window::_WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if( message == WM_NCCREATE )
    {
        auto pcreate = reinterpret_cast<CREATESTRUCTW*>( lParam );
        if( !pcreate )
        {
            exit( -1 );
        }

        SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( pcreate->lpCreateParams ) );
        return( DefWindowProc( hWnd, message, wParam, lParam ) );
    }
    else
    {
        auto *window = reinterpret_cast<Window*>( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
        if( window )
        {
            return( window->WindowProc( hWnd, message, wParam, lParam ) );
        }
    }

    return DefWindowProc( hWnd, message, wParam, lParam );
}

LRESULT CALLBACK Window::WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
    // Keyboard Messages
    case WM_CHAR:
    {
        uint8_t key = static_cast<uint8_t>( wParam );
        if( keyboard.IsCharAutoRepeat() )
        {
            keyboard.OnChar( key );
        }
        else
        {
            const bool was_pressed = lParam & ( 1 << 30 );
            if( !was_pressed )
            {
                keyboard.OnChar( key );
            }
        }
        break;
    }

    case WM_KEYDOWN:
    {
        uint8_t key = static_cast<uint8_t>( wParam );
        if( keyboard.IsEventAutoRepeat() )
        {
            keyboard.OnPressed( key );
        }
        else
        {
            const bool was_pressed = lParam & ( 1 << 30 );
            if( !was_pressed )
            {
                keyboard.OnPressed( key );
            }
        }
        break;
    }

    case WM_KEYUP:
    {
        uint8_t key = static_cast<uint8_t>( wParam );
        if( keyboard.IsEventAutoRepeat() )
        {
            keyboard.OnReleased( key );
        }
        else
        {
            const bool was_pressed = lParam & ( 1 << 30 );
            if( was_pressed )
            {
                keyboard.OnReleased( key );
            }
        }
        break;
    }

    // Mouse Messages
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        mouse.OnLeftPressed( x, y );
        break;
    }

    case WM_LBUTTONUP:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        mouse.OnLeftReleased( x, y );
        break;
    }

    case WM_RBUTTONDOWN:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        mouse.OnRightPressed( x, y );
        break;
    }

    case WM_RBUTTONUP:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        mouse.OnRightReleased( x, y );
        break;
    }

    case WM_MBUTTONDOWN:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        mouse.OnMiddlePressed( x, y );
        break;
    }

    case WM_MBUTTONUP:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        mouse.OnMiddleReleased( x, y );
        break;
    }

    case WM_MOUSEWHEEL:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        if( GET_WHEEL_DELTA_WPARAM( wParam ) > 0 )
        {
            mouse.OnWheelUp( x, y );
        }
        else if( GET_WHEEL_DELTA_WPARAM( wParam ) < 0 )
        {
            mouse.OnWheelDown( x, y );
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x = LOWORD( lParam );
        int y = HIWORD( lParam );
        mouse.OnMove( x, y );
        break;
    }

    case WM_INPUT:
    {
        UINT required_size;
        GetRawInputData( reinterpret_cast<HRAWINPUT>( lParam ), RID_INPUT, nullptr, &required_size, sizeof( RAWINPUTHEADER ) );
        raw_buffer.resize( required_size );

        if( GetRawInputData( reinterpret_cast<HRAWINPUT>( lParam ), RID_INPUT, &raw_buffer[ 0 ], &required_size, sizeof( RAWINPUTHEADER ) ) != required_size )
        {
            break;
        }

        auto raw = reinterpret_cast<RAWINPUT*>( &raw_buffer[ 0 ] );
        if( raw->header.dwType == RIM_TYPEMOUSE )
        {
            mouse.OnRawMove( raw->data.mouse.lLastX, raw->data.mouse.lLastY );
        }


        break;
    }

    // Other Messages
    case WM_COMMAND:
        {
            //int wmId = LOWORD(wParam);
            //switch (wmId)
            //{
            //default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            //}
        }
        break;

    case WM_SIZE:
    {
        UINT width = LOWORD( lParam );
        UINT height = HIWORD( lParam );
        graphics.OnWindowResized( (int)width, (int)height );
        break;
    }

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
