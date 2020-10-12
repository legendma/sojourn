#include "pch.h"
#include "GraphicsAdapters.h"
#include "Utils.h"

std::vector<GraphicsAdapters::Adapter> GraphicsAdapters::GetAdapters()
{
    if( adapters.size() > 0 )
    {
        return adapters;
    }

    ComPtr<IDXGIFactory> factory;
    Utils::ComThrow( CreateDXGIFactory( __uuidof(IDXGIFactory), reinterpret_cast<void**>( factory.GetAddressOf() ) ) );

    IDXGIAdapter *adapter = nullptr;
    UINT i = 0;
    while( SUCCEEDED( factory->EnumAdapters( i, &adapter ) ) )
    {
        adapters.push_back( Adapter( adapter ) );
        i++;
    }

    return adapters;
}

GraphicsAdapters::Adapter::Adapter( IDXGIAdapter *adapter ) :
    adapter( adapter )
{
    Utils::ComThrow( adapter->GetDesc( &description ) );

    UINT i = 0;
    IDXGIOutput *output;
    while( adapter->EnumOutputs( i, &output ) != DXGI_ERROR_NOT_FOUND )
    {
        outputs.push_back( AdapterOutput( output ) );
        ++i;
    }

}

GraphicsAdapters::AdapterOutput::AdapterOutput( IDXGIOutput *output )
{
    output->GetDesc( &description );
}

std::vector<DXGI_MODE_DESC> GraphicsAdapters::AdapterOutput::QueryModesByFormat( DXGI_FORMAT format )
{
    std::vector<DXGI_MODE_DESC> modes;
    
    UINT cnt = 0;
    output->GetDisplayModeList( format, 0, &cnt, 0 );

    DXGI_MODE_DESC desc = { 0 };
    modes.resize( cnt );
    output->GetDisplayModeList( format, 0, &cnt, &modes[ 0 ] );

    return modes;
}
