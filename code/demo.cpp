
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <windows.h>

#include <assert.h>

#include <dxgi.h>
#include <d3d11.h>

#include "demo.h"

void Demo::Init(ID3D11Device* d3d_device, ID3D11DeviceContext* d3d_device_context, IDXGISwapChain* dxgi_swap_chain)
{
}

void Demo::Tick(ID3D11Device* d3d_device, ID3D11DeviceContext* d3d_device_context, IDXGISwapChain* dxgi_swap_chain)
{
	HRESULT res_dxgi_swap_chain_present = dxgi_swap_chain->Present(1U, 0U);
	assert(SUCCEEDED(res_dxgi_swap_chain_present));
}