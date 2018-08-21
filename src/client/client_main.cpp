#include "pch.hpp"

#include "common/network/network_matchmaking.hpp"
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

    /* START - delete these lines */
    Engine::NetworkConnectionPassportRaw passport;
    if( !Engine::FAKE_NetworkGetPassport( passport ) )
        return 0;

    app->OnReceivedMatchmaking( passport );
    /* END - delete these lines */

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

    // Create the Networking Adapter
    if( !StartNetworking() )
    {
        return false;
    }

    // Create the application
    m_main = std::make_unique<T>( m_timer, m_graphics );
    
    return true;
}

template <typename T>
int Client::App<T>::Run()
{
    MSG msg = { 0 };
    /* REMOVE THIS */
    Engine::NetworkConnectionPassportRaw raw_passport;
    Engine::FAKE_NetworkGetPassport( raw_passport );
    Engine::NetworkConnectionPassportPtr passport( new Engine::NetworkConnectionPassport() );
    if( !passport->Read( raw_passport ) )
    {
        return -1;
    }

    m_connection->Connect( passport );
    /* END REMOVE THIS */

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
            // Update scene objects.
            m_timer.Tick( [&]()
            {
                UpdateNetworking();
                m_main->Update();
                m_main->Render();
                SendPacketsToServer(); // TODO IMPLEMENT
            } );
            
        }
    }

    return (int)msg.wParam;
}

template<typename T>
void Client::App<T>::Shutdown()
{
}

template<typename T>
bool Client::App<T>::StartNetworking()
{
    m_networking = Engine::NetworkingFactory::StartNetworking();
    if( m_networking == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Networking system could not be started.  Exiting..." );
        return false;
    }

    m_network_config.our_address = L"0.0.0.0";
    m_connection = Engine::NetworkConnectionFactory::CreateConnection( m_network_config, m_networking );
    if( m_networking == nullptr )
    {
        Engine::Log( Engine::LOG_LEVEL_ERROR, L"Networking connection could not be created.  Exiting..." );
        return false;
    }

    return true;
}

template<typename T>
void Client::App<T>::UpdateNetworking()
{
    m_connection->SendAndReceivePackets();
    HandleGamePacketsFromServer(); // TODO IMPLEMENT
}

template<typename T>
void Client::App<T>::ReceivePackets()
{
    Engine::NetworkPacketTypesAllowed allowed;
    allowed.SetAllowed( Engine::PACKET_CONNECT_DENIED );
    allowed.SetAllowed( Engine::PACKET_CONNECT_CHALLENGE );
    allowed.SetAllowed( Engine::PACKET_KEEP_ALIVE );
    allowed.SetAllowed( Engine::PACKET_PAYLOAD );
    allowed.SetAllowed( Engine::PACKET_DISCONNECT );

    auto now = m_timer.GetTotalTicks();
    byte data[NETWORK_MAX_PACKET_SIZE];
    while( true )
    {
        Engine::NetworkAddressPtr from;
        auto byte_cnt = m_socket->ReceiveFrom( data, sizeof( data ), from );
        if( byte_cnt == 0 )
            break;

        auto read = Engine::InputBitStreamFactory::CreateInputBitStream( data, byte_cnt, false );
        auto packet = m_networking->ReadPacket( m_network_config.protocol_id, m_crypto, m_network_config.private_key, allowed, now, read );
        if( packet )
        {
            ProcessPacket( packet, from, now );
        }
    }
}

template<typename T>
void Client::App<T>::ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time )
{
    switch( packet->packet_type )
    {
    case Engine::PACKET_CONNECT_CHALLENGE:
        OnReceivedConnectionChallenge( packet, from, now_time );
        break;
    }
}

template<typename T>
void Client::App<T>::UpdateConnection()
{
}

template<typename T>
void Client::App<T>::HandleGamePacketsFromServer()
{
}

template<typename T>
void Client::App<T>::SendPacketsToServer()
{
}

template<typename T>
void Client::App<T>::OnReceivedConnectionChallenge( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time )
{
}

template<typename T>
void Client::App<T>::OnReceivedMatchmaking( Engine::NetworkConnectionPassportRaw &raw )
{
    auto passport = Engine::NetworkConnectionPassportPtr( new Engine::NetworkConnectionPassport );
    if( !passport->Read( raw ) )
    {
        Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Client::App::OnReceivedMatchmaking Unable to read matchmaking packet." ).c_str() );
        return;
    }

    m_connection->Connect( passport );
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