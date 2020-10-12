#pragma once
#pragma comment( lib, "DXGI.lib" )

class GraphicsAdapters
{
public:
    struct AdapterOutput
    {
        ComPtr<IDXGIOutput> output;
        DXGI_OUTPUT_DESC description = { 0 };

        AdapterOutput( IDXGIOutput *output );
        std::vector<DXGI_MODE_DESC> QueryModesByFormat( DXGI_FORMAT format );
    };

    struct Adapter
    {
        ComPtr<IDXGIAdapter> adapter;
        DXGI_ADAPTER_DESC description = { 0 };
        bool supports_msaa4x = false;
        bool supports_msaa8x = false;
        std::vector<AdapterOutput> outputs;
        Adapter( IDXGIAdapter *adapter );
    };

    std::vector<Adapter> GetAdapters();
private:
    std::vector<Adapter> adapters;
};

