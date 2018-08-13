#include "pch.hpp"

//#include "engine/engine_dx_helper.hpp"
#include "explorer_main.hpp"

using namespace Game;
using namespace Windows::Foundation;
//using namespace Windows::System::Threading;
using namespace Concurrency;

// Loads and initializes application assets when the application is loaded.
Game::SojournExplorer::SojournExplorer( Engine::StepTimer &timer, const std::shared_ptr<Engine::GraphicsAdapter>& graphics )
                  : Game::Main( graphics, timer )
{

	// TODO: Replace this with your app's content initialization.


	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

Game::SojournExplorer::~SojournExplorer()
{

}

// Updates application state when the window size changes (e.g. device orientation change)
void Game::SojournExplorer::CreateWindowSizeDependentResources()
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

// Updates the application state once per frame.
void Game::SojournExplorer::Update()
{
	// Update scene objects.
	//m_timer.Tick([&]()
	//{
		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
        m_guiRenderer->Update(m_timer);
	//});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool Game::SojournExplorer::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_graphics->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_graphics->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_graphics->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_graphics->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView( m_graphics->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView( m_graphics->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	m_sceneRenderer->Render();
    m_guiRenderer->Render();

    m_graphics->Present();

	return true;
}

// Notifies renderers that device resources need to be released.
void Game::SojournExplorer::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
    m_guiRenderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void Game::SojournExplorer::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
    m_guiRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
