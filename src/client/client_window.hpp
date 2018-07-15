
namespace Client
{
    enum RawDevices
    {
        RAW_KEYBOARD,
        RAW_MOUSE,
        RAW_COUNT /* number of raw devices */
    };

    class Window
    {
    public:
        Window();
        bool CreateMainWindow( HINSTANCE hinstance );
        static LRESULT CALLBACK MainWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

        float GetDPI()  { return m_dpi; }
        int GetWidth()  { return m_window_width; }
        int GetHeight() { return m_window_height; }
        HWND GetHwnd() { return m_hwnd; }

     protected:
        std::wstring m_window_caption;
        int m_window_width;
        int m_window_height;
        bool m_is_resizing;
        HINSTANCE m_hinstance;
        HWND m_hwnd;
        float m_dpi;

    private:
        static class Window *m_this;
        LRESULT WindowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

        /* window messaging */
        virtual void OnActivate( bool is_active ) = 0;
        virtual void OnWindowSizeChanged( int width, int height ) = 0;
        virtual void OnVisibilityChanged( bool visible ) = 0;
        virtual void OnMouse( RAWMOUSE mouse ) = 0;
        virtual void OnKeyboard( RAWKEYBOARD keyboard ) = 0;
        virtual void OnDPIChanged( uint32_t new_dpi ) = 0;
    };
}
