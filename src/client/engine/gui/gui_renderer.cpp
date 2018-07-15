#include "pch.hpp"

#include "common/engine/engine_utilities.hpp"
#include "gui_renderer.hpp"

using namespace Game;
using namespace Microsoft::WRL;

// Initializes D2D resources used for text rendering.
GUIRenderer::GUIRenderer(const std::shared_ptr<Engine::GraphicsAdapter>& graphics ) :
	m_text(L""),
	m_graphics(graphics)
{
	ZeroMemory(&m_textMetrics, sizeof(DWRITE_TEXT_METRICS));

	// Create device independent resources
	ComPtr<IDWriteTextFormat> textFormat;
    Engine::ComThrow(
        m_graphics->GetDWriteFactory()->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_LIGHT,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			32.0f,
			L"en-US",
			&textFormat
			)
		);

	Engine::ComThrow(
		textFormat->QueryInterface( &m_textFormat )
		);

	Engine::ComThrow(
		m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
		);

	Engine::ComThrow(
        m_graphics->GetD2DFactory()->CreateDrawingStateBlock(&m_stateBlock)
		);

	CreateDeviceDependentResources();
}

// Updates the text to be displayed.
void GUIRenderer::Update( Engine::StepTimer const& timer)
{
	// Update display text.
	uint32_t fps = timer.GetFramesPerSecond();

	m_text = (fps > 0) ? std::to_wstring(fps) + L" FPS" : L" - FPS";

	ComPtr<IDWriteTextLayout> textLayout;
	Engine::ComThrow(
        m_graphics->GetDWriteFactory()->CreateTextLayout(
			m_text.c_str(),
			(uint32_t) m_text.length(),
			m_textFormat,
			240.0f, // Max width of the input text.
			50.0f, // Max height of the input text.
			&textLayout
			)
		);

	Engine::ComThrow(
		textLayout->QueryInterface( &m_textLayout.p )
		);

	Engine::ComThrow(
		m_textLayout->GetMetrics(&m_textMetrics)
		);
}

// Renders a frame to the screen.
void GUIRenderer::Render()
{
	ID2D1DeviceContext* context = m_graphics->GetD2DDeviceContext();
	auto logicalSize = m_graphics->GetLogicalSize();

	context->SaveDrawingState(m_stateBlock);
	context->BeginDraw();

	// Position on the bottom right corner
	D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
		logicalSize.Width - m_textMetrics.layoutWidth,
		logicalSize.Height - m_textMetrics.height
		);

	context->SetTransform(screenTranslation * m_graphics->GetOrientationTransform2D());

	Engine::ComThrow(
		m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING)
		);

	context->DrawTextLayout(
		D2D1::Point2F(0.f, 0.f),
		m_textLayout,
		m_whiteBrush
		);

	// Ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
	// is lost. It will be handled during the next call to Present.
	HRESULT hr = context->EndDraw();
	if (hr != D2DERR_RECREATE_TARGET)
	{
		Engine::ComThrow(hr);
	}

	context->RestoreDrawingState(m_stateBlock);
}

void GUIRenderer::CreateDeviceDependentResources()
{
	Engine::ComThrow(
        m_graphics->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush)
		);
}
void GUIRenderer::ReleaseDeviceDependentResources()
{
	m_whiteBrush.Release();
}