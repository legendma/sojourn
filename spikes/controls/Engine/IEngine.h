#pragma once
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"

enum class GameStateType;
class EntityWorld;
interface IEngine
{
    virtual Keyboard & GetKeyboard() = 0;
    virtual Mouse & GetMouse() = 0;
    virtual Graphics & GetGraphics() = 0;
    virtual void ChangeState( const GameStateType new_state ) = 0;
    virtual void AddWorld( EntityWorld *world ) = 0;
    virtual void RemoveWorld( EntityWorld *world ) = 0;
};