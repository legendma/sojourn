#pragma once


struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

// Data
int const                    NUM_FRAMES_IN_FLIGHT = 3;
extern FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT];
extern UINT                         g_frameIndex;

int const                    NUM_BACK_BUFFERS = 3;
extern ID3D12Device* g_pd3dDevice;
extern ID3D12DescriptorHeap* g_pd3dRtvDescHeap;
extern ID3D12DescriptorHeap* g_pd3dSrvDescHeap;
extern ID3D12CommandQueue* g_pd3dCommandQueue;
extern ID3D12GraphicsCommandList* g_pd3dCommandList;
extern ID3D12Fence* g_fence;
extern HANDLE                       g_fenceEvent;
extern UINT64                       g_fenceLastSignaledValue;
extern IDXGISwapChain3* g_pSwapChain;
extern HANDLE                       g_hSwapChainWaitableObject;
extern ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS];
extern D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS];

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();