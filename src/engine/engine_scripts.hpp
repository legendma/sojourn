#pragma once

#include "selene/selene.h"

namespace Engine
{
    class Scripts
    {
    public:
        Scripts();
        ~Scripts();

        void LoadScript( std::wstring script_name );
        void Bootstrap( void );

    protected:
        Selene::State   m_luaState;
    };
}
