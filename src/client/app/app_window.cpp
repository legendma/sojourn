#include "pch.hpp"

#include "app_window.hpp"

Client::Window *Client::Window::m_this_window = nullptr;

Client::Window::Window( HINSTANCE hInstance ) :
    m_hinstance( hInstance ),
    m_window_caption( L"Sojourn Client" ),
    m_window_width( 800 ),
    m_window_height( 600 ),
    m_dpi( 96 )
{
    Client::Window::m_this_window = this;
}

LRESULT CALLBACK
Client::Window::MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    // Forward hwnd on because we can get messages (e.g., WM_CREATE)
    // before CreateWindow returns, and thus before m_hwnd is valid.
    return m_this_window->WindowProc( hWnd, message, wParam, lParam );
}

bool Client::Window::CreateMainWindow()
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Client::Window::MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hinstance;
    wc.hIcon = LoadIcon( 0, IDI_APPLICATION );
    wc.hCursor = LoadCursor( 0, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"SojournWindowClassName";

    if( !RegisterClass( &wc ) )
    {
        MessageBox( 0, L"RegisterClass Failed.", 0, 0 );
        return false;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = { 0, 0, m_window_width, m_window_height };
    AdjustWindowRect( &R, WS_OVERLAPPEDWINDOW, false );
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    m_hwnd = CreateWindow( L"SojournWindowClassName", m_window_caption.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hinstance, 0 );
    if( !m_hwnd )
    {
        MessageBox( 0, L"CreateWindow Failed.", 0, 0 );
        return false;
    }

    /* register the mouse and keyboard */
    //RAWINPUTDEVICE rid[RAW_COUNT];
    //
    //rid[RAW_KEYBOARD].usUsagePage = 0x01;
    //rid[RAW_KEYBOARD].usUsage     = 0x06;
    //rid[RAW_KEYBOARD].dwFlags     = RIDEV_NOLEGACY;   // adds HID keyboard and also ignores WM_KEY* messages
    //rid[RAW_KEYBOARD].hwndTarget  = 0;
    //
    //rid[RAW_MOUSE].usUsagePage    = 0x01;
    //rid[RAW_MOUSE].usUsage        = 0x02;
    //rid[RAW_MOUSE].dwFlags        = RIDEV_NOLEGACY;   // adds HID mouse and also ignores WM_MOUSE* messages
    //rid[RAW_MOUSE].hwndTarget     = 0;
    //
    //if( RegisterRawInputDevices( rid, RAW_COUNT, sizeof( RAWINPUTDEVICE ) ) == FALSE )
    //{
    //    // TODO: registration failed. Call GetLastError for the cause of the error
    //}

    ShowWindow( m_hwnd, SW_SHOW );
    UpdateWindow( m_hwnd );

    return true;
}

LRESULT Client::Window::WindowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        // WM_ACTIVATE is sent when the window is activated or deactivated.  
    case WM_ACTIVATE:
        if( LOWORD( wParam ) == WA_INACTIVE )
        {
            OnActivate( false );
        }
        else
        {
            OnActivate( true );
        }
        return 0;

        // WM_SIZE is sent when the user resizes the window.  
    case WM_SIZE:
        // Save the new client area dimensions.
        m_window_width = LOWORD( lParam );
        m_window_height = HIWORD( lParam );
        if( wParam == SIZE_MINIMIZED )
        {
            OnVisibilityChanged( false );
        }
        else if( wParam == SIZE_MAXIMIZED )
        {
            OnVisibilityChanged( true );
        }
        else if( wParam == SIZE_RESTORED )
        {
            if( m_is_resizing )
            {
                // If user is dragging the resize bars, we do not resize 
                // the buffers here because as the user continuously 
                // drags the resize bars, a stream of WM_SIZE messages are
                // sent to the window, and it would be pointless (and slow)
                // to resize for each WM_SIZE message received from dragging
                // the resize bars.  So instead, we reset after the user is 
                // done resizing the window and releases the resize bars, which 
                // sends a WM_EXITSIZEMOVE message.
            }
            else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
            {
                OnVisibilityChanged( true );
            }
        }
        return 0;

        // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        OnVisibilityChanged( false );
        return 0;

        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
        // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
        OnVisibilityChanged( true );
        OnWindowSizeChanged( m_window_width, m_window_height );
        return 0;

        // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage( 0 );
        return 0;

        // The WM_MENUCHAR message is sent when a menu is active and the user presses 
        // a key that does not correspond to any mnemonic or accelerator key. 
    case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT( 0, MNC_CLOSE );

        // Catch this message so to prevent the window from becoming too small.
    case WM_GETMINMAXINFO:
        ( (MINMAXINFO*)lParam )->ptMinTrackSize.x = 200;
        ( (MINMAXINFO*)lParam )->ptMinTrackSize.y = 200;
        return 0;

    case WM_INPUT:
    {
        UINT bufferSize = 40;
        BYTE *buffer = new BYTE[ 40 ]; // mouse = 40, keyboard = 32

        GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, (LPVOID)buffer, &bufferSize, sizeof( RAWINPUTHEADER ) );
        RAWINPUT *raw = (RAWINPUT*)buffer;

        // Check what the input is: mouse or keyboard?
        if( raw->header.dwType == RIM_TYPEMOUSE ) // mouse
        {
            OnMouse( raw->data.mouse );
            // raw->data.mouse.lLastX contains the relative X position of the mouse
            // raw->data.mouse.lLastY contains the relative Y position of the mouse
            // (raw->data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN) will be 0 when left mouse button is not down.
        }
        else if( raw->header.dwType == RIM_TYPEKEYBOARD ) //keyboard
        {
            OnKeyboard( raw->data.keyboard );
            // Get key value from the keyboard member (of type RAWKEYBOARD)
            //USHORT keyCode = raw->data.keyboard.VKey;
        }

    }
    default:
        break;
    }

    return DefWindowProc( hwnd, msg, wParam, lParam );
}



