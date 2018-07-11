#include "pch.hpp"

#include "engine_resources.hpp"
#include "engine_utilities.hpp"
#include "graphics_shaders.hpp"
#include "graphics_utilities.hpp"

using namespace Engine;

Shader::Shader( const std::shared_ptr<Engine::GraphicsAdapter>& graphics, std::wstring &filename ) :
	m_loadingComplete( false ),
	m_graphics( graphics ),
	m_filename( filename.append( L".cso" ) )
{
}

VertexShader::VertexShader( const std::shared_ptr<Engine::GraphicsAdapter>& graphics, std::wstring &filename ) :
	Shader( graphics, filename )
{
	CreateDeviceDependentResources();
}

void VertexShader::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
    auto shaders = Engine::Resources::OpenFolder( RESOURCE_FOLDER_SHADERS );
    auto vs_task = shaders->ReadDataAsync( m_filename );

	// After the vertex shader file is loaded, create the shader and input layout.
    vs_task.then( [this]( const std::vector<byte>& fileData ) {
		Engine::ComThrow(
			m_graphics->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		    { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

        Engine::ComThrow(
			m_graphics->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE( vertexDesc ),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	} ).then( [this]() {
		CD3D11_BUFFER_DESC constantBufferDesc( sizeof( ModelViewProjectionConstantBuffer ), D3D11_BIND_CONSTANT_BUFFER );
			Engine::ComThrow(
				m_graphics->GetD3DDevice()->CreateBuffer(
					&constantBufferDesc,
					nullptr,
					&m_constantBuffer
				)
			);

	} ).then( [this]() {
		// Once the shader is created, mark as loaded
		m_loadingComplete = true;
	} );
}

void VertexShader::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Release();
	m_inputLayout.Release();
	m_constantBuffer.Release();
}

void VertexShader::Stage()
{
	// Loading is asynchronous. Only use after loading is complete
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_graphics->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer,
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
	);

	context->IASetInputLayout( m_inputLayout );

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader,
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		&m_constantBuffer.p,
		nullptr,
		nullptr
	);
}

PixelShader::PixelShader( const std::shared_ptr<Engine::GraphicsAdapter>& graphics, std::wstring &filename ) :
	Shader( graphics, filename )
{
	CreateDeviceDependentResources();
}

void PixelShader::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
    auto shaders = Engine::Resources::OpenFolder( RESOURCE_FOLDER_SHADERS );
	auto psTask = shaders->ReadDataAsync( m_filename );

	// After the pixel shader file is loaded, create the shader and constant buffer.
	psTask.then([this](const std::vector<byte>& fileData) {
        Engine::ComThrow(
			m_graphics->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
				)
			);
	}).then([this] () {
		m_loadingComplete = true;
	});
}

void PixelShader::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_pixelShader.Release();
}

void PixelShader::Stage()
{
	// Loading is asynchronous. Only use after loading is complete
	if(!m_loadingComplete)
	{
		return;
	}

	auto context = m_graphics->GetD3DDeviceContext();

	context->PSSetShader(
		m_pixelShader,
		nullptr,
		0
	);
}