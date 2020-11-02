#include "pch.h"
#include "PlayerConnectionSystem.h"

class ConnectionComponent;
class InputStreamComponent;

void PlayerConnectionSystem::Update( float timestep )
{
    auto view = world.GetTuple<ConnectionComponent, InputStreamComponent>();
    //for( auto entity : view )
    //{
    //    auto connection   = entity.Get<ConnectionComponent>();
    //    auto input_stream = entity.Get<InputStreamComponent>();
    //}
}
