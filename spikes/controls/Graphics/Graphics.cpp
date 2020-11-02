#include "pch.h"
#include "Graphics.h"
#include "Utils.h"

Graphics::~Graphics()
{
    if( swapchain )
    {
        swapchain->SetFullscreenState( false, nullptr );
    }
}

bool Graphics::Initialize( HWND hwnd )
{
    if( !InitializeDirectX( hwnd, true ) )
    {
        return false;
    }

    is_initialized = true;

    return true;
}

std::shared_ptr<RenderTarget> Graphics::CreateRenderTarget( int width, int height )
{
    auto new_target = std::make_shared<RenderTarget>( *this, width, height );
    render_targets.push_back( new_target );
    return new_target;
}

void Graphics::OnWindowResized( int width, int height )
{
    // Ignore if we are not initialized
    if( !is_initialized )
    {
        return;
    }

    window_width  = width;
    window_height = height;

    if( is_windowed_mode )
    {
        ResizeBackbuffer();
    }
}

void Graphics::ListenDeviceLost( DeviceLostNotify *listener )
{
    device_lost_listeners.push_back( listener );
}

void Graphics::Present()
{
    // flip the backbuffer
    HRESULT result;
    {
        unsigned int vsync_intervals = 0;
        if( is_vsync )
        {
            vsync_intervals = 1;
        }

        DXGI_PRESENT_PARAMETERS params = { 0 };
        result = swapchain->Present1( vsync_intervals, 0, &params );
    }

    // clear the render target
    context->DiscardView1( render_target.Get(), nullptr, 0 );

    // clear the depth buffer
    context->DiscardView1( depth_stencil.Get(), nullptr, 0 );

    // check for success, or device lost
    if( result == DXGI_ERROR_DEVICE_REMOVED
     || result == DXGI_ERROR_DEVICE_RESET )
    {
        OnDeviceLost();
    }
    else
    {
        Utils::ComThrow( result );
    }
}

void Graphics::Clear( float r, float g, float b, float a )
{
    DirectX::XMFLOAT4 color( r, g, b, a );
    context->ClearRenderTargetView( render_target.Get(), reinterpret_cast<FLOAT*>( &color ) );
}

void Graphics::Clear( bool clear_depth, float depth, bool clear_stencil, uint32_t stencil )
{
    UINT flags = 0;
    if( clear_depth )
    {
        flags |= D3D11_CLEAR_DEPTH;
    }

    if( clear_stencil )
    {
        flags |= D3D11_CLEAR_STENCIL;
    }

    context->ClearDepthStencilView( depth_stencil.Get(), flags, depth, stencil );
}

std::list<Graphics::MSAAModeType> Graphics::GetSupportedMSAA()
{
    std::list<Graphics::MSAAModeType> modes;
    modes.push_back( MSAAModeType::MSAA_NONE );
    auto current_adapter = GetCurrentAdapter();
    if( current_adapter )
    {
        if( current_adapter->supports_msaa4x )
        {
            modes.push_back( MSAAModeType::MSAA_4X );
        }

        if( current_adapter->supports_msaa8x )
        {
            modes.push_back( MSAAModeType::MSAA_8X );
        }
    }
    
    return modes;
}

void Graphics::SetMSAA( MSAAModeType mode )
{
    // sanity check that adapter supports the requested mode
    if( auto current_adapter = GetCurrentAdapter() )
    {
        switch( mode )
        {
            case MSAAModeType::MSAA_4X:
                if( !current_adapter->supports_msaa4x )
                {
                    // TODO log an error
                    return;
                }
                break;

            case MSAAModeType::MSAA_8X:
                if( !current_adapter->supports_msaa8x )
                {
                    // TODO log an error
                    return;
                }
                break;
        }
    }

    auto previous_mode = msaa_mode;
    msaa_mode = mode;
    if( previous_mode != msaa_mode )
    {
        // create a new swap chain with the requested MSAA level
        CreateSwapChain();
    }
}

bool Graphics::InitializeDirectX( HWND hwnd, bool start_windowed_mode )
{
    is_windowed_mode = start_windowed_mode;
    this->hwnd = hwnd;

    // Enumerate the adapters
    auto found_adapters = adapters.GetAdapters();
    if( found_adapters.empty() )
    {
        return false;
    }

    // Create the device
    if( !CreateDevice() )
    {
        return false;
    }

    return true;
}

bool Graphics::CreateDevice()
{
    adapter_index = -1;
    device.Reset();

    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined( _DEBUG )
    // Enable driver debug messages
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL request_level = D3D_FEATURE_LEVEL_11_1;
    D3D_FEATURE_LEVEL got_level = {};

    // Try each adapter until we find one that supports DirectX 11.1 features
    int index = 0;
    for( auto &adapter : adapters.GetAdapters() )
    {
        ComPtr<ID3D11Device> created_device;
        ComPtr<ID3D11DeviceContext> created_context;
        auto hr = D3D11CreateDevice( adapter.adapter.Get(), 
                                     D3D_DRIVER_TYPE_UNKNOWN,
                                     0,
                                     flags,
                                     &request_level, 1,
                                     D3D11_SDK_VERSION,
                                     created_device.GetAddressOf(),
                                     &got_level,
                                     created_context.GetAddressOf() );

        if( FAILED( hr ) )
        {
            _com_error error( hr );
            auto error_text = error.ErrorMessage();
            index++;
            continue;
        }

        // store pointers to the Direct3D 11.3 API device and immediate context.
        Utils::ComThrow( created_device.As( &device ) );
        Utils::ComThrow( created_context.As( &context ) );

        // Check MSAA 4x and 8x support
        device->CheckMultisampleQualityLevels( GetColorFormat(), 4, &adapter.quality_4x );
        adapter.supports_msaa4x = adapter.quality_4x > 0;

        device->CheckMultisampleQualityLevels( GetColorFormat(), 8, &adapter.quality_8x );
        adapter.supports_msaa8x = adapter.quality_8x > 0;

        adapter_index = index;
        break;
    }

    return device.Get() != nullptr;
}

void Graphics::CreateSwapChain()
{
    swapchain = nullptr;
    DXGI_SWAP_CHAIN_DESC desc = { 0 };
    desc.BufferDesc.Width = backbuffer_width;
    desc.BufferDesc.Height = backbuffer_height;
    desc.BufferDesc.Format = GetColorFormat();
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.RefreshRate.Numerator = target_framerate;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.OutputWindow = hwnd;
    desc.Windowed = is_windowed_mode;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // multi-sampling
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    auto current_adapter = GetCurrentAdapter();
    if( current_adapter )
    {
        switch( msaa_mode )
        {
        case MSAAModeType::MSAA_4X:
            assert( current_adapter->supports_msaa4x );
            desc.SampleDesc.Count = 4;
            desc.SampleDesc.Quality = current_adapter->quality_4x;
            break;

        case MSAAModeType::MSAA_8X:
            assert( current_adapter->supports_msaa8x );
            desc.SampleDesc.Count = 8;
            desc.SampleDesc.Quality = current_adapter->quality_8x;
            break;
        }
    }

    ComPtr<IDXGIDevice3> dxgi_device;
    Utils::ComThrow( device.As( &dxgi_device ) );
    ComPtr<IDXGIAdapter> dxgi_adapter;
    Utils::ComThrow( dxgi_device->GetAdapter( dxgi_adapter.GetAddressOf() ) );

    ComPtr<IDXGIFactory4> dxgi_factory;
    dxgi_adapter->GetParent( IID_PPV_ARGS( &dxgi_factory ) );

    ComPtr<IDXGISwapChain> new_swapchain;
    dxgi_factory->CreateSwapChain( device.Get(), &desc, new_swapchain.GetAddressOf() );
    new_swapchain.As( &swapchain );    
}

void Graphics::ResizeBackbuffer()
{
    render_target = nullptr;
    depth_stencil = nullptr;
    context->Flush();

    // update the backbuffer size
    if( is_windowed_mode )
    {
        backbuffer_width  = std::max( window_width, 1 );
        backbuffer_height = std::max( window_height, 1 );
    }

    if( swapchain )
    {
        // swap chain exists already, so resize it
        auto result = swapchain->ResizeBuffers( 2, (UINT)backbuffer_width, (UINT)backbuffer_height, GetColorFormat(), 0 );

        if( result == DXGI_ERROR_DEVICE_REMOVED
         || result == DXGI_ERROR_DEVICE_RESET )
        {
            // device not available, so create a new device and swap chain
            OnDeviceLost();
            return;
        }
        else
        {
            // check result
            Utils::ComThrow( result );
        }
    }
    else
    {
        CreateSwapChain();
    }

    // create the backbuffer render target
    {
        ComPtr<ID3D11Texture2D1> texture;
        Utils::ComThrow( swapchain->GetBuffer( 0, IID_PPV_ARGS( &texture ) ) );
        Utils::ComThrow( device->CreateRenderTargetView1( texture.Get(), nullptr, render_target.GetAddressOf() ) );
    }    

    // create the backbuffer depth/stencil
    {
        ComPtr<ID3D11Texture2D1> texture;
        auto texture_desc = CD3D11_TEXTURE2D_DESC1( GetDepthFormat(), backbuffer_width, backbuffer_height, 1, 1, D3D11_BIND_DEPTH_STENCIL );
        Utils::ComThrow( device->CreateTexture2D1( &texture_desc, nullptr, texture.GetAddressOf() ) );

        auto view_desc = CD3D11_DEPTH_STENCIL_VIEW_DESC( D3D11_DSV_DIMENSION_TEXTURE2D );
        Utils::ComThrow( device->CreateDepthStencilView( texture.Get(), &view_desc, depth_stencil.GetAddressOf() ) );
    }

    // resize the viewport
    {
        auto viewport = CD3D11_VIEWPORT( 0.0f, 0.0f, (float)backbuffer_width, (float)backbuffer_height );
        context->RSSetViewports( 1, &viewport );
    }
}

void Graphics::OnDeviceLost()
{
    swapchain = nullptr;
    device = nullptr;
    context = nullptr;

    for( auto it : device_lost_listeners )
    {
        it->OnDeviceLost();
    }

    CreateDevice();
    ResizeBackbuffer();

    for( auto it : device_lost_listeners )
    {
        it->OnDeviceRestored();
    }
}

GraphicsAdapters::Adapter * Graphics::GetCurrentAdapter()
{
    auto found = adapters.GetAdapters();
    try
    {
        return &found.at( adapter_index );
    }
    catch( std::out_of_range ) {}

    return nullptr;
}

