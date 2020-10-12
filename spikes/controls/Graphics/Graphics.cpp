#include "pch.h"
#include "Graphics.h"

bool Graphics::Initialize( HWND hwnd, int width, int height )
{
    backbuffer_width  = width;
    backbuffer_height = height;
    if( !InitializeDirectX( hwnd, width, height ) )
    {
        return false;
    }

    return true;
}

std::shared_ptr<RenderTarget> Graphics::CreateRenderTarget( int width, int height )
{
    auto new_target = std::make_shared<RenderTarget>( *this, width, height );
    render_targets.push_back( new_target );

    return new_target;
}

bool Graphics::InitializeDirectX( HWND hwnd, int width, int height )
{
    // Enumerate the adapters
    auto found_adapters = adapters.GetAdapters();
    if( found_adapters.size() == 0 )
    {
        return false;
    }

    // Create the device
    if( !CreateDevice() )
    {
        return false;
    }

    return false;
}

bool Graphics::CreateDevice()
{
    adapter_index = -1;
    device.Reset();

    UINT flags = 0;
#if defined( _DEBUG )
    // Enable driver debug messages
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL request_level = D3D_FEATURE_LEVEL_11_1;
    D3D_FEATURE_LEVEL got_level = {};

    // Try each adapter until we find one that supports DirectX 11.1
    int index = 0;
    for( auto &adapter : adapters.GetAdapters() )
    {
        auto hr = D3D11CreateDevice( adapter.adapter.Get(), 
                                     D3D_DRIVER_TYPE_HARDWARE,
                                     0,
                                     flags,
                                     &request_level, 1,
                                     D3D11_SDK_VERSION,
                                     device.GetAddressOf(),
                                     &got_level,
                                     context.GetAddressOf() );

        if( FAILED( hr ) )
        {
            index++;
            continue;
        }

        // Check MSAA 4x and 8x support
        UINT quality = 0;
        device->CheckMultisampleQualityLevels( GetTextureFormat(), 4, &quality );
        adapter.supports_msaa4x = quality > 0;

        device->CheckMultisampleQualityLevels( GetTextureFormat(), 8, &quality );
        adapter.supports_msaa8x = quality > 0;

        adapter_index = index;
        break;
    }

    return device.Get() != nullptr;
}

void Graphics::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    //desc.
}

