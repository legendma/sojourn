#pragma once
#include "System.h"
#include "EntityWorld.h"

class PlayerConnectionSystem final : public System
{
public:
    virtual void Update( float timestep );

protected:
    EntityWorld world;
};

