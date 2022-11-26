#pragma once

namespace ecs
{

template <typename T>
class System
{
    bool is_paused = false;

public:
    void DoFrame();
    void Pause( bool should_pause = true ) { is_paused = should_pause; }
};

template <typename T>
System<T>::DoFrame()
{
if( is_paused )
    {
    return;
    }

T::Proc();

}

} // namespace ecs