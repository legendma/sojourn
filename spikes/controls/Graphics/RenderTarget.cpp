#include "pch.h"
#include "RenderTarget.h"
#include "Graphics.h"
#include "Utils.h"

RenderTarget::RenderTarget( Graphics &graphics, const int width, const int height ) :
    graphics( graphics )
{
    CreateTarget( width, height );
}

void RenderTarget::Set()
{
    auto context = graphics.GetDeviceContext();
    ComPtr<ID3D11RenderTargetView> rt;
    Utils::ComThrow( target.As( &rt ) );
    context->OMSetRenderTargets( 1, rt.GetAddressOf(), graphics.GetDepthStencil().Get() );
}

void RenderTarget::Clear( float r, float g, float b, float a )
{
    DirectX::XMFLOAT4 color( r, g, b, a );
    graphics.GetDeviceContext()->ClearRenderTargetView( target.Get(), reinterpret_cast<FLOAT*>( &color ) );
}

void RenderTarget::CreateTarget( const int width, const int height )
{
    auto device = graphics.GetDevice();

    // Create the texture
    auto tex_desc = CD3D11_TEXTURE2D_DESC1( graphics.GetColorFormat(), 
                                            static_cast<UINT>( width ),
                                            static_cast<UINT>( height ),
                                            1,
                                            1,
                                            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE );

    Utils::ComThrow( device->CreateTexture2D1( &tex_desc, nullptr, texture.GetAddressOf() ) );
    
    Utils::ComThrow( device->CreateRenderTargetView1( texture.Get(), nullptr, target.GetAddressOf() ) );

    // Create shader resource
    /*D3D11_SHADER_RESOURCE_VIEW_DESC resource_desc = {};
    resource_desc.Format = tex_desc.Format;
    resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resource_desc.Texture2D.MipLevels = 1;*/

    Utils::ComThrow( device->CreateShaderResourceView1( texture.Get(), nullptr, resource.GetAddressOf() ) );
}
