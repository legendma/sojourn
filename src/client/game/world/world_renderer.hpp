#pragma once

//#include "..\Common\DeviceResources.h"
#include "common/engine/engine_step_timer.hpp"
#include "game/shaders/shaders_structures.hpp"
//#include "..\Engine\Shader.h"

namespace Game
{
	// This sample renderer instantiates a basic rendering pipeline.
	class World3DRenderer
	{
	public:
		World3DRenderer(const std::shared_ptr<Engine::GraphicsAdapter>& graphics);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(Engine::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }


	private:
		void Rotate(float radians);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<Engine::GraphicsAdapter> m_graphics;

		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;

		// System resources for cube geometry.
		Engine::ModelViewProjectionConstantBuffer	m_constantBufferData;
		std::shared_ptr<Engine::VertexShader> m_vertexShader;
		std::shared_ptr<Engine::PixelShader> m_pixelShader;
		//DirectX::XMFLOAT4X4 m_model;
		uint32_t	m_indexCount;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;
	};
}

