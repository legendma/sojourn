#pragma once
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "DirectXTK.lib" )

#include "GraphicsAdapters.h"
#include "RenderTarget.h"

class Graphics
{
public:
    Graphics() = default;
    bool Initialize( HWND hwnd, int width, int height );
    std::shared_ptr<RenderTarget> CreateRenderTarget( int width, int height );

    inline ComPtr<ID3D11Device> GetDevice() { return device; }
    inline ComPtr<ID3D11DeviceContext> GetDeviceContext() { return context; }
    inline ComPtr<ID3D11DepthStencilView> GetDepthStencil() { return depth_stencil; }
    inline int GetBackBufferWidth() { return backbuffer_width; }
    inline int GetBackBufferHeight() { return backbuffer_height; }
    inline DXGI_FORMAT GetTextureFormat() { return texture_format; }

protected:
    bool InitializeDirectX( HWND hwnd, int width, int height );
    bool CreateDevice();
    void CreateSwapChain();

private:
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11RenderTargetView> render_target_view;
    ComPtr<ID3D11DepthStencilView> depth_stencil;
    int backbuffer_width;
    int backbuffer_height;
    std::list<std::shared_ptr<RenderTarget>> render_targets;
    GraphicsAdapters adapters;
    int adapter_index = -1;

    static const DXGI_FORMAT texture_format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

