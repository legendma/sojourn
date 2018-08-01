
#include "client_window.hpp"
#include "common/network/network_main.hpp"
#include "common/engine/engine_step_timer.hpp"
#include "engine/network/network_client_config.hpp"
#include "engine/network/network_client_connection.hpp"

namespace Client
{
    interface IClientApp
    {
    virtual bool Start( HINSTANCE ) = 0;
    virtual int Run() = 0;
    virtual void Shutdown() = 0;
    };

    std::shared_ptr<Client::IClientApp> CreateApp();

    template <typename T>
    class App : public IClientApp,
                public Client::Window
    {
    friend std::shared_ptr<Client::IClientApp> CreateApp();
    public:

        // IClientApp
        virtual bool Start( HINSTANCE hinstance );
        virtual int Run();
        virtual void Shutdown();

     protected:
        std::unique_ptr<T> m_main;
        Engine::GraphicsAdapterPtr m_graphics;
        Engine::StepTimer m_timer;

        bool m_is_active;
        bool m_is_visible;


        /* window messaging */
        virtual void OnActivate( bool is_active );
        virtual void OnWindowSizeChanged( int width, int height );
        virtual void OnVisibilityChanged( bool visible );
        virtual void OnMouse( RAWMOUSE mouse );
        virtual void OnKeyboard( RAWKEYBOARD keyboard );
        virtual void OnDPIChanged( uint32_t new_dpi );

        /* networking */
        Engine::NetworkingPtr m_networking;
        Engine::NetworkConnectionPtr m_connection;
        Engine::NetworkClientConfig m_network_config;
        Engine::NetworkCryptoMapPtr m_crypto;

        bool StartNetworking();
        void UpdateNetworking();
        void ReceivePackets();

        void ProcessPacket( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time );
        void UpdateConnection();
        void HandleGamePacketsFromServer();
        void SendPacketsToServer();

        void OnReceivedConnectionChallenge( Engine::NetworkPacketPtr &packet, Engine::NetworkAddressPtr &from, uint64_t &now_time );

    private:
        App();
    };
}
