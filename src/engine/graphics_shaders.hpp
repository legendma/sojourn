#pragma once

#include "engine/graphics_adapter.hpp"
#include "game/shaders/shaders_structures.hpp" // TODO: <MPA> REMOVE THIS INCLUDE

namespace Engine
{
	// Base Shader
	class Shader
	{
	public:
		Shader( const std::shared_ptr<Engine::GraphicsAdapter>& deviceResources, std::wstring &filename );
		bool IsLoaded() { return m_loadingComplete;  }
		virtual void CreateDeviceDependentResources() {};
		//virtual void CreateWindowSizeDependentResources() {};
		virtual void ReleaseDeviceDependentResources() {};
		virtual void Stage() {};

	protected:
		std::shared_ptr<Engine::GraphicsAdapter> m_graphics;
		bool	                             m_loadingComplete;
		std::wstring                         m_filename;
	};

	// Vertex Shader
	class VertexShader : public Shader
	{
	public:
		VertexShader( const std::shared_ptr<Engine::GraphicsAdapter>& deviceResources, std::wstring &filename );
		virtual void CreateDeviceDependentResources();
		virtual void ReleaseDeviceDependentResources();
		virtual void Stage();
		void SetTransforms( ModelViewProjectionConstantBuffer& transforms ) { m_constantBufferData = transforms; }

	protected:
		CComPtr<ID3D11VertexShader>	m_vertexShader;
		CComPtr<ID3D11Buffer>		m_constantBuffer;
		CComPtr<ID3D11InputLayout>	m_inputLayout;
		ModelViewProjectionConstantBuffer	        m_constantBufferData;
	};

	// Pixel Shader
	class PixelShader : public Shader
	{
	public:
		PixelShader( const std::shared_ptr<Engine::GraphicsAdapter>& graphics, std::wstring &filename );
		virtual void CreateDeviceDependentResources();
		virtual void ReleaseDeviceDependentResources();
		virtual void Stage();

	protected:
		CComPtr<ID3D11PixelShader>	m_pixelShader;
	};
}
