#pragma once


#include "game/game_main.h"

// Renders Direct2D and 3D content on the screen.
namespace Game
{
	class SojournExplorer : public Game::Main
	{
	public:
        SojournExplorer( const std::shared_ptr<Engine::GraphicsAdapter>& graphics );
		~SojournExplorer();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:

	};
}