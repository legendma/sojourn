
#include "client_window.hpp"
#include "common/network/network_main.hpp"

#define DEFAULT_CLIENT_SOCKET_SNDBUF_SIZE ( 256 * 1024 )
#define DEFAULT_CLIENT_SOCKET_RCVBUF_SIZE ( 256 * 1024 )

namespace Client
{
    struct NetworkConfig
    {
        uint64_t protocol_id;
        Engine::NetworkKey private_key;
        size_t send_buff_size;
        size_t receive_buff_size;
        //int server_fps;
        std::wstring server_address;
        //unsigned int max_num_clients;

        NetworkConfig() :
            protocol_id( NETWORK_SOJOURN_PROTOCOL_ID ),
            send_buff_size( DEFAULT_CLIENT_SOCKET_SNDBUF_SIZE ),
            receive_buff_size( DEFAULT_CLIENT_SOCKET_RCVBUF_SIZE )
        {
            Engine::Networking::GenerateEncryptionKey( private_key );
        };
    };


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
        NetworkConfig m_network_config;
        Engine::NetworkSocketUDPPtr m_socket;
        Engine::NetworkAddressPtr m_server_address;
        Engine::NetworkCryptoMapPtr m_crypto;

        bool StartNetworking( std::wstring our_address );
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
