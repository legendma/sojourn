
#include "app_window.hpp"

namespace Framework
{
    interface IFrameworkApp
    {
    virtual bool Start( HINSTANCE ) = 0;
    virtual int Run() = 0;
    virtual void Shutdown() = 0;
    };

    std::shared_ptr<Framework::IFrameworkApp> CreateApp();

    template <typename T>
    class App : public  IFrameworkApp,
                public Framework::Window
    {
    friend std::shared_ptr<Framework::IFrameworkApp> CreateApp();
    public:

        // IFrameworkSignals
        virtual bool Start( HINSTANCE hinstance );
        virtual int Run();
        virtual void Shutdown();

     protected:
        std::unique_ptr<T> m_main;
        std::shared_ptr<Engine::GraphicsAdapter>  m_graphics;
        bool m_is_active;
        bool m_is_visible;

        /* window messaging */
        virtual void OnActivate( bool is_active );
        virtual void OnWindowSizeChanged( int width, int height );
        virtual void OnVisibilityChanged( bool visible );
        virtual void OnMouse( RAWMOUSE mouse );
        virtual void OnKeyboard( RAWKEYBOARD keyboard );
        virtual void OnDPIChanged( uint32_t new_dpi );

    private:
        App();
    };
}
