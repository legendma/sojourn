#include "pch.hpp"

#include "engine_resources.hpp"
#include "engine_scripts.hpp"

using namespace Engine;
using namespace Selene;

std::wstring WideCharFromChar( std::string string )
{
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    return( converter.from_bytes( string ) );
}


Scripts::Scripts() : m_luaState(true)
{
    Bootstrap();
}

Scripts::~Scripts()
{
}

void Scripts::LoadScript( std::wstring script_name )
{
    script_name.append( L".lua" );
    auto folder = Engine::Resources::OpenFolder( RESOURCE_FOLDER_SCRIPTS );
    auto file_task = folder->ReadDataAsync( script_name );
    auto success = file_task.then( [script_name, this]( std::vector<byte> data ) -> bool
    {
        return m_luaState.LoadBinary( script_name, &data[0], static_cast<size_t>( data.size() ) );
    }
    ).get();
    
    assert( success );

}

void Scripts::Bootstrap( void )
{
    // Do the bootstrap
    LoadScript( L"bootstrap" );
    int number_scripts = m_luaState["Bootstrap"]["num_of_scripts"]();
    auto scripts = m_luaState["Bootstrap"]["scripts"];
    for( int i = 0; i < number_scripts; i++ )
    {
        std::wstring script = WideCharFromChar( scripts[i] );
        std::size_t found = script.find( L".lua" );
        if( found != std::string::npos )
            script = script.substr( 0, found );
        LoadScript( script );
    }
}
