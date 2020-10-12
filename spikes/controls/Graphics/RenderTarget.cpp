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
    context->OMSetRenderTargets( 1, target.GetAddressOf(), graphics.GetDepthStencil().Get() );
}

void RenderTarget::CreateTarget( const int width, const int height )
{
    auto device = graphics.GetDevice();

    // Create the texture
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = static_cast<UINT>( width );
    tex_desc.Height = static_cast<UINT>( height );
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = graphics.GetTextureFormat();
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    Utils::ComThrow( device->CreateTexture2D( &tex_desc, NULL, texture.GetAddressOf() ) );

    // Create render target view
    D3D11_RENDER_TARGET_VIEW_DESC target_desc = {};
    target_desc.Format = tex_desc.Format;
    target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    Utils::ComThrow( device->CreateRenderTargetView( texture.Get(), &target_desc, target.GetAddressOf() ) );

    // Create shader resource
    D3D11_SHADER_RESOURCE_VIEW_DESC resource_desc = {};
    resource_desc.Format = tex_desc.Format;
    resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resource_desc.Texture2D.MipLevels = 1;

    Utils::ComThrow( device->CreateShaderResourceView( texture.Get(), &resource_desc, resource.GetAddressOf() ) );
}
