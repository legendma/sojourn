#pragma once

#include "game/gui/gui_renderer.hpp"
#include "game/world/world_renderer.hpp"
#include "engine/engine_device_resources.hpp"
#include "engine/engine_scripts.hpp"
#include "engine/engine_step_timer.hpp"

namespace Game
{
    class Main : public Engine::IDeviceNotify
    {
    public:
        Main( const std::shared_ptr<Engine::GraphicsAdapter>& graphics );
        ~Main();

        // IDeviceNotify
        virtual void OnDeviceLost();
        virtual void OnDeviceRestored();

     private:
         void StartupSystem(void);

         // Cached pointer to device resources.

     protected:
        // Rendering loop timer.
	    Engine::StepTimer m_timer;
        // TODO: Replace with your own content renderers.
        std::unique_ptr<Game::World3DRenderer>     m_sceneRenderer;
        std::unique_ptr<Game::GUIRenderer>         m_guiRenderer;
        std::shared_ptr<Engine::GraphicsAdapter>   m_graphics;
        std::shared_ptr<Engine::Scripts>           m_scripting;
    };
}
