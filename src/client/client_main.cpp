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

    // Create the Networking Adapter
    m_network_config.server_address = L"127.0.0.1:48000";
    m_networking = Engine::NetworkingFactory::StartNetworking();
    if( m_networking == nullptr
     || !StartNetworking( std::wstring( L"0.0.0.0" ) ) )
    {
        Engine::ReportError( L"Networking system could not be started.  Exiting..." );
        return false;
    }

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
            // Update scene objects.
            m_timer.Tick( [&]()
            {
                UpdateNetworking();
                m_main->Update();
                if( m_is_active )
                {
                m_main->Render();
                }
                else
                {
                    Sleep( 100 );
                }
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
bool Client::App<T>::StartNetworking( std::wstring our_address )
{
    // Create the server address
    m_server_address = Engine::NetworkAddressFactory::CreateAddressFromStringAsync( m_network_config.server_address ).get();
    if( m_server_address == nullptr )
    {
        Engine::ReportError( L"Client::App::StartNetworking Given server address is invalid!" );
        throw std::runtime_error( "CreateAddressFromStringAsync" );
    }

    // Create the server socket
    m_socket = Engine::NetworkSocketUDPFactory::CreateUDPSocket( m_server_address, m_network_config.receive_buff_size, m_network_config.send_buff_size );
    if( m_socket == nullptr )
    {
        Engine::ReportError( L"Client::App::StartNetworking Unable to create client socket." );
        throw std::runtime_error( "CreateUDPSocket" );
    }

    //Engine::Log( Engine::LOG_LEVEL_DEBUG, std::wstring( L"Server listening on %s" ).c_str(), m_server_address->Print() );
    
    // create the challenge key
    //Engine::Networking::GenerateEncryptionKey( m_challenge_key );

    return true;
}

template<typename T>
void Client::App<T>::UpdateNetworking()
{
    ReceivePackets();
    UpdateConnection(); // TODO IMPLEMENT
    HandleGamePacketsFromServer(); // TODO IMPLEMENT
    SendPacketsToServer(); // TODO IMPLEMENT
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
        //ReadAndProcessPacket( m_network_config.protocol_id, allowed, from, now, read );
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