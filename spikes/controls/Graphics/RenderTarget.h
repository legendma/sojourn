#pragma once

class Graphics;
class RenderTarget
{
public:
    RenderTarget( Graphics &graphics, const int width, const int height );
    void Set();
    ComPtr<ID3D11ShaderResourceView> GetShaderResource() { return resource; }
    void Clear( float r, float g, float b, float a );

protected:
    void CreateTarget( const int width, const int height );

private:
    ComPtr<ID3D11Texture2D1> texture;
    ComPtr<ID3D11RenderTargetView1> target;
    ComPtr<ID3D11ShaderResourceView1> resource;
    Graphics &graphics;
};

