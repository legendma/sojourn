#pragma once

#include "game_entity.hpp"

namespace Game
{
    class GamePlayer : public Game::GameEntity<GamePlayer, Game::PLAYER_ENTITY>
    {
    friend class GameEntityManager;
    private:
        GamePlayer();

    };
}