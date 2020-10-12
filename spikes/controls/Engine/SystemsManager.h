#pragma once
#include "EntityWorld.h"

class SystemsManager
{
public:
    void Tick( const float timestep, EntityWorld &world );
};

