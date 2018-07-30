#pragma once

#include "common/engine/engine_scripts.hpp"
#include "common/engine/engine_step_timer.hpp"

#include "engine/graphics/graphics_adapter.hpp"
#include "engine/gui/gui_renderer.hpp" // TODO <MPA>: Rename/Reorganize these once they are given a purpose besides testing
#include "game/world/world_renderer.hpp"

namespace Game
{
    class Main : public Engine::IDeviceNotify
    {
    public:
        Main( const std::shared_ptr<Engine::GraphicsAdapter> &graphics, Engine::StepTimer &timer );
        ~Main();

        // IDeviceNotify
        virtual void OnDeviceLost();
        virtual void OnDeviceRestored();

     private:
         void StartupSystem(void);

         // Cached pointer to device resources.

     protected:
         // Rendering loop timer.
         Engine::StepTimer &m_timer;

        // TODO: Replace with your own content renderers.
        std::unique_ptr<Game::World3DRenderer>     m_sceneRenderer;
        std::unique_ptr<Game::GUIRenderer>         m_guiRenderer;
        std::shared_ptr<Engine::GraphicsAdapter>   m_graphics;
        std::shared_ptr<Engine::Scripts>           m_scripting;
    };
}
