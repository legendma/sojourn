#include "pch.hpp"

#include "common/engine/engine_utilities.hpp"

#include "game/explorer/explorer_main.hpp"
#include "client_main.hpp"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    auto app = Client::CreateApp();
    if ( !app->Start( hInstance) )
        return 0;

    auto ret = app->Run();
    app->Shutdown();
    return( ret );
}

template <typename T>
Client::App<T>::App() : 
    m_is_active( false ), 
    m_is_visible( false )
{
}

template <typename T>
bool Client::App<T>::Start( HINSTANCE hinstance )
{
    // TODO get the monitors DPI via GetDpiForMonitor and MDT_EFFECTIVE_DPI, see https://docs.microsoft.com/en-us/windows/desktop/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor
    // and https://docs.microsoft.com/en-us/windows/desktop/api/windef/ne-windef-dpi_awareness

    // Create the main application window
    if( !CreateMainWindow( hinstance ) )
    {
        return false;
    }

    // Create the Graphics Adapter
    Engine::Log( Engine::LOG_LEVEL_DEBUG, L"Starting DirectX..." );
    m_graphics = std::make_unique<Engine::GraphicsAdapter>();
    auto window = std::shared_ptr<Client::Window>( this );
    m_graphics->SetWindow( window );

    // Create the application
    m_main = std::make_unique<T>( m_graphics );
    
    return true;
}

template <typename T>
int Client::App<T>::Run()
{
    MSG msg = { 0 };
    //mTimer.Reset();

    while( msg.message != WM_QUIT )
    {
        // If there are Window messages then process them.
        if( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        // Otherwise, do animation/game stuff.
        else
        {
            //mTimer.Tick();
            if( m_is_active )
            {
                m_main->Update();
                m_main->Render();
            }
            else
            {
                Sleep( 100 );
            }
        }
    }

    return (int)msg.wParam;
}

template<typename T>
void Client::App<T>::Shutdown()
{
}

template<typename T>
void Client::App<T>::OnActivate( bool is_active )
{
    m_is_active = is_active;
}

template<typename T>
void Client::App<T>::OnWindowSizeChanged( int width, int height )
{
    // TODO: send this signal to the render device
    m_graphics->SetLogicalSize( Engine::Size( (float)width, (float)height ) );
}

template<typename T>
void Client::App<T>::OnVisibilityChanged( bool visible )
{
    // we don't want to draw if we are not visible
    m_is_visible = visible;
}

template<typename T>
void Client::App<T>::OnMouse( RAWMOUSE mouse )
{
    // only process mouse and keyboard input if we are active
    if( !m_is_active )
        return;
}

template<typename T>
void Client::App<T>::OnKeyboard( RAWKEYBOARD keyboard )
{
    // only process mouse and keyboard input if we are active
    if( !m_is_active )
        return;
}

template<typename T>
void Client::App<T>::OnDPIChanged( uint32_t new_dpi )
{
    // TODO: Handle DPI changing by setting the dips scaling on the render device
    //https://www.discussiongenerator.com/2014/01/16/resolution-independent-2d-rendering-in-sdl2/
}

std::shared_ptr<Client::IClientApp> Client::CreateApp()
{
    return std::shared_ptr<Client::IClientApp>( new Client::App<Game::SojournExplorer>() );
}