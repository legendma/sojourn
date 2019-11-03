#include "app_window.hpp"

#include "common/engine/engine_step_timer.hpp"
#include "common/engine/network/network_main.hpp"
#include "engine/network/network_connection.hpp"

#include "engine/graphics/graphics_adapter.hpp"

namespace Client
{
    class Application : public Window, public Engine::IGraphicsWindow
    {
    public:
        Application( HINSTANCE hInstance ) : 
            Window( hInstance ) {}

        bool Start();
        int Run();
        void Shutdown();

        /* IGraphicsWindow */
        virtual HWND GetHwnd() { return m_hwnd; }

    protected:
        /* window messaging */
        virtual void OnActivate( bool is_active );
        virtual void OnWindowSizeChanged( int width, int height );
        virtual void OnVisibilityChanged( bool visible );
        virtual void OnMouse( RAWMOUSE mouse );
        virtual void OnKeyboard( RAWKEYBOARD keyboard );
        virtual void OnDPIChanged( uint32_t new_dpi );

    private:
        bool m_is_active;
        bool m_is_visible;

        Engine::GraphicsAdapterPtr m_graphics;
        Engine::StepTimer m_timer;

        /* networking */
        Engine::NetworkingPtr m_networking;
        Engine::NetworkConnectionPtr m_connection;
        Engine::NetworkClientConfig m_network_config;
        Engine::NetworkCryptoMapPtr m_crypto;

        bool StartNetworking();
        void UpdateNetworking();
        //void ReceivePackets();

        void ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time );
        void UpdateConnection();
        void HandleGamePacketsFromServer();
        void SendPacketsToServer();

        void OnReceivedConnectionChallenge( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time );
        void OnReceivedMatchmaking( Engine::NetworkConnectionPassportRaw &raw );
    };
}