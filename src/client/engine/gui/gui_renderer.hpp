#pragma once
#include <string>

#include "common/engine/engine_step_timer.hpp"
#include "engine/graphics/graphics_adapter.hpp"

namespace Game
{
	// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
	class GUIRenderer
	{
	public:
		GUIRenderer(const std::shared_ptr<Engine::GraphicsAdapter>& graphics);
		void CreateDeviceDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(Engine::StepTimer const& timer);
		void Render();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<Engine::GraphicsAdapter> m_graphics;

		// Resources related to text rendering.
		std::wstring                                    m_text;
		DWRITE_TEXT_METRICS	                            m_textMetrics;
		CComPtr<ID2D1SolidColorBrush>                   m_whiteBrush;
		CComPtr<ID2D1DrawingStateBlock1>                m_stateBlock;
		CComPtr<IDWriteTextLayout3>                     m_textLayout;
		CComPtr<IDWriteTextFormat2>                     m_textFormat;
	};
}