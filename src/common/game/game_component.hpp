#pragma once

#include "game_main.hpp"

namespace Game
{
    interface IComponent
    {
    };

    class GameComponentManager
    {
    public:
        void RemoveAllComponents( GameEntityId entity_id );
    }; typedef std::shared_ptr<GameComponentManager> GameComponentManagerPtr;
}