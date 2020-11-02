#pragma once
#pragma comment( lib, "d3d11.lib" )

#include "GraphicsAdapters.h"
#include "RenderTarget.h"

class Graphics
{
public:
    class DeviceLostNotify
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };

    enum class MSAAModeType
    {
        MSAA_NONE,
        MSAA_4X,
        MSAA_8X
    };

public:
    Graphics() = default;
    ~Graphics();
    bool Initialize( HWND hwnd );
    std::shared_ptr<RenderTarget> CreateRenderTarget( int width, int height );
    void OnWindowResized( int width, int height );
    void ListenDeviceLost( DeviceLostNotify *listener );
    void Present();
    void Clear( float r, float g, float b, float a );
    void Clear( bool clear_depth, float depth, bool clear_stencil, uint32_t stencil );
    std::list<MSAAModeType> GetSupportedMSAA();
    void SetMSAA( MSAAModeType mode );

    inline ComPtr<ID3D11Device3> GetDevice() { return device; }
    inline ComPtr<ID3D11DeviceContext3> GetDeviceContext() { return context; }
    inline ComPtr<ID3D11DepthStencilView> GetDepthStencil() { return depth_stencil; }
    inline int GetBackBufferWidth() { return backbuffer_width; }
    inline int GetBackBufferHeight() { return backbuffer_height; }
    inline ComPtr<ID3D11RenderTargetView1> GetBackbuffer() { return render_target; }
    inline DXGI_FORMAT GetColorFormat() { return color_format; }
    inline DXGI_FORMAT GetDepthFormat() { return depth_stencil_format; }
    inline void SetVSync( bool set ) { is_vsync = set; }
    inline bool GetVSync() { return is_vsync; }

protected:
    bool InitializeDirectX( HWND hwnd, bool start_windowed_mode );
    bool CreateDevice();
    void CreateSwapChain();
    void ResizeBackbuffer();
    void OnDeviceLost();
    GraphicsAdapters::Adapter * GetCurrentAdapter();

private:
    ComPtr<ID3D11Device3> device;
    ComPtr<ID3D11DeviceContext3> context;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D11RenderTargetView1> render_target;
    ComPtr<ID3D11DepthStencilView> depth_stencil;
    int backbuffer_width = 0;
    int backbuffer_height = 0;
    HWND hwnd;
    int window_width = 0;
    int window_height = 0;
    int fullscreen_width = 0;
    int fullscreen_height = 0;
    std::list<std::shared_ptr<RenderTarget>> render_targets;
    GraphicsAdapters adapters;
    int adapter_index = -1;
    bool is_initialized = false;
    bool is_windowed_mode = false;
    std::list<DeviceLostNotify*> device_lost_listeners;
    MSAAModeType msaa_mode = MSAAModeType::MSAA_NONE;
    bool is_vsync = false;

    static const DXGI_FORMAT color_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    static const DXGI_FORMAT depth_stencil_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    static const int target_framerate = 60;
};

