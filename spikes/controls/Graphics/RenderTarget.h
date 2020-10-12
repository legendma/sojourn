#pragma once

class Graphics;
class RenderTarget
{
public:
    RenderTarget( Graphics &graphics, const int width, const int height );
    void Set();
    ComPtr<ID3D11ShaderResourceView> GetShaderResource() { return resource; }

protected:
    void CreateTarget( const int width, const int height );

private:
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11ShaderResourceView> resource;
    Graphics &graphics;
};

