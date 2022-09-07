#ifndef _DEMO_H_
#define _DEMO_H_ 1

#include <sdkddkver.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>

#include <dxgi.h>
#include <d3d11.h>

class Demo
{
public:
	void Init(ID3D11Device *d3d11_device, ID3D11DeviceContext* d3d11_device_context, IDXGISwapChain* dxgi_swap_chain);
	void Tick(ID3D11Device* d3d11_device, ID3D11DeviceContext *d3d11_device_context, IDXGISwapChain* dxgi_swap_chain);
};

#endif