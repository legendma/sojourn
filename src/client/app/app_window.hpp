namespace Client
{
    class Window
    {
    public:
        Window( HINSTANCE hInstance );

        static LRESULT CALLBACK MainWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

    private:
        static class Window *m_this_window;

        LRESULT WindowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

    protected:
        std::wstring m_window_caption;
        int m_window_width;
        int m_window_height;
        bool m_is_resizing;
        HINSTANCE m_hinstance;
        HWND m_hwnd;
        float m_dpi;

        bool CreateMainWindow();

        /* window messaging */
        virtual void OnActivate( bool is_active ) = 0;
        virtual void OnWindowSizeChanged( int width, int height ) = 0;
        virtual void OnVisibilityChanged( bool visible ) = 0;
        virtual void OnMouse( RAWMOUSE mouse ) = 0;
        virtual void OnKeyboard( RAWKEYBOARD keyboard ) = 0;
        virtual void OnDPIChanged( uint32_t new_dpi ) = 0;
    
    };
}