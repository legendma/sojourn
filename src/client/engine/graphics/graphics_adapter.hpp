#pragma once

namespace Client
{
    class Window;
}

namespace Engine
{
    // Forward declarations
	class VertexShader;
	class PixelShader;

    struct Size
    {
        float Width;
        float Height;
        Size() : Width( 0.0f ), Height( 0.0f ) {};
        Size( float width, float height ) : Width( width ), Height( height ) {};
        bool operator == ( Size &other ) const { return( ( Width == other.Width ) && ( Height == other.Height ) ) ; }
        bool operator != ( Size &other ) const { return( !( *this == other ) ); }

    };

	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	// Controls all the DirectX device resources.
	class GraphicsAdapter : public std::enable_shared_from_this<GraphicsAdapter>
	{
	public:
        GraphicsAdapter();
		void SetWindow( std::shared_ptr<Client::Window> &window);
		void SetLogicalSize( Engine::Size logicalSize);
		//void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void ValidateDevice();
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void Trim();
		void Present();

		// The size of the render target, in pixels.
        Engine::Size	            GetOutputSize() const					{ return m_outputSize; }

		// The size of the render target, in dips.
        Engine::Size	            GetLogicalSize() const					{ return m_logicalSize; }
		float						GetDpi() const							{ return m_effectiveDpi; }

		// D3D Accessors.
		ID3D11Device3*				GetD3DDevice() const					{ return m_d3dDevice; }
		ID3D11DeviceContext3*		GetD3DDeviceContext() const				{ return m_d3dContext; }
		IDXGISwapChain3*			GetSwapChain() const					{ return m_swapChain; }
		D3D_FEATURE_LEVEL			GetDeviceFeatureLevel() const			{ return m_d3dFeatureLevel; }
		ID3D11RenderTargetView1*	GetBackBufferRenderTargetView() const	{ return m_d3dRenderTargetView; }
		ID3D11DepthStencilView*		GetDepthStencilView() const				{ return m_d3dDepthStencilView; }
		D3D11_VIEWPORT				GetScreenViewport() const				{ return m_screenViewport; }
		DirectX::XMFLOAT4X4			GetOrientationTransform3D() const		{ return m_orientationTransform3D; }

		// D2D Accessors.
		ID2D1Factory3*				GetD2DFactory() const					{ return m_d2dFactory; }
		ID2D1Device2*				GetD2DDevice() const					{ return m_d2dDevice; }
		ID2D1DeviceContext2*		GetD2DDeviceContext() const				{ return m_d2dContext; }
		ID2D1Bitmap1*				GetD2DTargetBitmap() const				{ return m_d2dTargetBitmap; }
		IDWriteFactory3*			GetDWriteFactory() const				{ return m_dwriteFactory; }
		//IWICImagingFactory2*		GetWicImagingFactory() const			{ return m_wicFactory.Get(); }
		D2D1::Matrix3x2F			GetOrientationTransform2D() const		{ return m_orientationTransform2D; }

		// Shader System
		std::shared_ptr<Engine::VertexShader> GetVertexShader( std::wstring name );
		std::shared_ptr<Engine::PixelShader> GetPixelShader( std::wstring name );

	private:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void UpdateRenderTargetSize();
		DXGI_MODE_ROTATION ComputeDisplayRotation();

		// Direct3D objects.
		CComPtr<ID3D11Device3>			m_d3dDevice;
		CComPtr<ID3D11DeviceContext3>	m_d3dContext;
		CComPtr<IDXGISwapChain3>        m_swapChain;

		// Direct3D rendering objects. Required for 3D.
		CComPtr<ID3D11RenderTargetView1>    m_d3dRenderTargetView;
		CComPtr<ID3D11DepthStencilView>     m_d3dDepthStencilView;
		D3D11_VIEWPORT                      m_screenViewport;

		// Direct2D drawing components.
		CComPtr<ID2D1Factory3>          m_d2dFactory;
		CComPtr<ID2D1Device2>           m_d2dDevice;
		CComPtr<ID2D1DeviceContext2>    m_d2dContext;
		CComPtr<ID2D1Bitmap1>           m_d2dTargetBitmap;

		// DirectWrite drawing components.
		CComPtr<IDWriteFactory3>		m_dwriteFactory;
		//CComPtr<IWICImagingFactory2>	m_wicFactory;

		// Cached reference to the Window.
        std::shared_ptr<Client::Window> m_window;

		// Cached device properties.
		D3D_FEATURE_LEVEL								m_d3dFeatureLevel;
		Engine::Size			              			m_d3dRenderTargetSize;
		Engine::Size			              			m_outputSize;
		Engine::Size			              			m_logicalSize;
		/*Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;*/
		float											m_dpi;

		// This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
		float m_effectiveDpi;

		// Transforms used for display orientation.
		D2D1::Matrix3x2F	m_orientationTransform2D;
		DirectX::XMFLOAT4X4	m_orientationTransform3D;

		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		IDeviceNotify* m_deviceNotify;

		// Shader System
		std::map<std::wstring, std::shared_ptr<Engine::VertexShader>> m_vertexShaders;
		std::map<std::wstring, std::shared_ptr<Engine::PixelShader>> m_pixelShaders;
	};
    typedef std::shared_ptr<GraphicsAdapter> GraphicsAdapterPtr;
}