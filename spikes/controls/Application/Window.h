#pragma once
class Window
{
public:
    using Factory = std::function<std::unique_ptr<Window>( int width, int height )>;
    Window( int width, int height );
    
    virtual bool ProcessMessages();

    uint16_t GetWidth() { return window_width; }
    uint16_t GetHeight() { return window_height; }

protected:
    uint16_t window_width = 0;
    uint16_t window_height = 0;
};

