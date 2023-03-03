
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <windows.h>

#include <stdint.h>
#include <assert.h>
#include <cmath>
#include <algorithm>

#include <DirectXMath.h>

#include <dxgi.h>
#include <d3d11.h>

#include "support/resolution.h"

#include "support/camera_controller.h"

#include "demo.h"

#include "ltc_lut_data.h"

#include "../shaders/plane_vs.hlsl.inl"

#include "../shaders/plane_fs.hlsl.inl"

#include "../shaders/rect_light_vs.hlsl.inl"

#include "../shaders/rect_light_fs.hlsl.inl"

#include "../shaders/post_process_vs.hlsl.inl"

#include "../shaders/post_process_fs.hlsl.inl"

struct plane_uniform_buffer_per_frame_binding_t
{
	// mesh
	DirectX::XMFLOAT4X4 model_transform;
	DirectX::XMFLOAT3 dcolor;
	float _padding_dcolor;
	DirectX::XMFLOAT3 scolor;
	float _padding_scolor;
	float roughness;
	float _padding_roughness_1;
	float _padding_roughness_2;
	float _padding_roughness_3;

	// camera
	DirectX::XMFLOAT4X4 view_transform;
	DirectX::XMFLOAT4X4 projection_transform;
	DirectX::XMFLOAT3 eye_position;
	float _padding_eye_position;

	// light
	DirectX::XMFLOAT4 rect_light_vetices[4];
	float intensity;
	float culling_range;
	float __padding_align16_1;
	float __padding_align16_2;
};

struct rect_light_uniform_buffer_per_frame_binding_t
{
	// camera
	DirectX::XMFLOAT4X4 view_transform;
	DirectX::XMFLOAT4X4 projection_transform;

	// light
	DirectX::XMFLOAT4 rect_light_vetices[4];
	float intensity;
	float __padding_align16_1;
	float __padding_align16_2;
	float __padding_align16_3;
};

static int8_t float_to_snorm(float unpacked_input);

uint32_t ReverseBits(uint32_t InValue)
{
	static uint8_t const ByteReversal[256] = {
		0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
		8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
		4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
		12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
		2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
		10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
		6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
		14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
		1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
		9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
		5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
		13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
		3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
		11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
		7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
		15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255};

	uint8_t Byte0 = ByteReversal[InValue >> 0 * 8 & 0xff];
	uint8_t Byte1 = ByteReversal[InValue >> 1 * 8 & 0xff];
	uint8_t Byte2 = ByteReversal[InValue >> 2 * 8 & 0xff];
	uint8_t Byte3 = ByteReversal[InValue >> 3 * 8 & 0xff];

	uint32_t OutValue = Byte0 << 3 * 8 | Byte1 << 2 * 8 | Byte2 << 1 * 8 | Byte3 << 0 * 8;
	return OutValue;
}

void Demo::Init(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, IDXGISwapChain *dxgi_swap_chain)
{
	m_attachment_backbuffer_rtv = NULL;
	{
		ID3D11Texture2D *attachment_backbuffer = NULL;
		HRESULT res_dxgi_swap_chain_get_buffer = dxgi_swap_chain->GetBuffer(0U, IID_PPV_ARGS(&attachment_backbuffer));
		assert(SUCCEEDED(res_dxgi_swap_chain_get_buffer));

		D3D11_RENDER_TARGET_VIEW_DESC d3d_render_target_view_desc;
		d3d_render_target_view_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		d3d_render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		d3d_render_target_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_d3d_device_create_render_target_view = d3d_device->CreateRenderTargetView(attachment_backbuffer, &d3d_render_target_view_desc, &m_attachment_backbuffer_rtv);
		assert(SUCCEEDED(res_d3d_device_create_render_target_view));
	}

	m_plane_vb_position = NULL;
	{
		float vb_position_data[] = {
			-7777.0, 0.0, 7777.0,
			7777.0, 0.0, 7777.0,
			-7777.0, 0.0, -7777.0,
			7777.0, 0.0, -7777.0};

		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(vb_position_data);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = vb_position_data;
		d3d_subresource_data.SysMemPitch = sizeof(vb_position_data);
		d3d_subresource_data.SysMemSlicePitch = sizeof(vb_position_data);

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, &d3d_subresource_data, &m_plane_vb_position);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	m_plane_vb_varying = NULL;
	{
		float vb_varying_data[] = {
			0.0, 1.0, 0.0,
			0.0, 1.0, 0.0,
			0.0, 1.0, 0.0,
			0.0, 1.0, 0.0};

		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(vb_varying_data);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = vb_varying_data;
		d3d_subresource_data.SysMemPitch = sizeof(vb_varying_data);
		d3d_subresource_data.SysMemSlicePitch = sizeof(vb_varying_data);

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, &d3d_subresource_data, &m_plane_vb_varying);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	m_plane_vao = NULL;
	{
		D3D11_INPUT_ELEMENT_DESC d3d_input_elements_desc[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			};

		HRESULT res_d3d_create_input_layout = d3d_device->CreateInputLayout(
			d3d_input_elements_desc,
			sizeof(d3d_input_elements_desc) / sizeof(d3d_input_elements_desc[0]),
			plane_vs_bytecode,
			sizeof(plane_vs_bytecode),
			&m_plane_vao);
		assert(SUCCEEDED(res_d3d_create_input_layout));
	}

	m_plane_vs = NULL;
	{
		HRESULT res_d3d_device_create_vertex_shader = d3d_device->CreateVertexShader(plane_vs_bytecode, sizeof(plane_vs_bytecode), NULL, &m_plane_vs);
		assert(SUCCEEDED(res_d3d_device_create_vertex_shader));
	}

	m_plane_fs = NULL;
	{
		HRESULT res_d3d_device_create_pixel_shader = d3d_device->CreatePixelShader(plane_fs_bytecode, sizeof(plane_fs_bytecode), NULL, &m_plane_fs);
		assert(SUCCEEDED(res_d3d_device_create_pixel_shader));
	}

	m_plane_rs = NULL;
	{
		D3D11_RASTERIZER_DESC d3d_rasterizer_desc;
		d3d_rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		d3d_rasterizer_desc.CullMode = D3D11_CULL_BACK;
		d3d_rasterizer_desc.FrontCounterClockwise = TRUE;
		d3d_rasterizer_desc.DepthBias = 0;
		d3d_rasterizer_desc.SlopeScaledDepthBias = 0.0f;
		d3d_rasterizer_desc.DepthBiasClamp = 0.0f;
		d3d_rasterizer_desc.DepthClipEnable = TRUE;
		d3d_rasterizer_desc.ScissorEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		d3d_rasterizer_desc.AntialiasedLineEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		HRESULT res_d3d_device_create_rasterizer_state = d3d_device->CreateRasterizerState(&d3d_rasterizer_desc, &m_plane_rs);
		assert(SUCCEEDED(res_d3d_device_create_rasterizer_state));
	}

	m_plane_uniform_buffer_per_frame_binding = NULL;
	{
		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(plane_uniform_buffer_per_frame_binding_t);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, NULL, &m_plane_uniform_buffer_per_frame_binding);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	m_rect_light_vao = NULL;

	m_rect_light_vs = NULL;
	{
		HRESULT res_d3d_device_create_vertex_shader = d3d_device->CreateVertexShader(rect_light_vs_bytecode, sizeof(rect_light_vs_bytecode), NULL, &m_rect_light_vs);
		assert(SUCCEEDED(res_d3d_device_create_vertex_shader));
	}

	m_rect_light_fs = NULL;
	{
		HRESULT res_d3d_device_create_pixel_shader = d3d_device->CreatePixelShader(rect_light_fs_bytecode, sizeof(rect_light_fs_bytecode), NULL, &m_rect_light_fs);
		assert(SUCCEEDED(res_d3d_device_create_pixel_shader));
	}

	m_rect_light_rs = NULL;
	{
		D3D11_RASTERIZER_DESC d3d_rasterizer_desc;
		d3d_rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		d3d_rasterizer_desc.CullMode = D3D11_CULL_BACK;
		d3d_rasterizer_desc.FrontCounterClockwise = FALSE;
		d3d_rasterizer_desc.DepthBias = 0;
		d3d_rasterizer_desc.SlopeScaledDepthBias = 0.0f;
		d3d_rasterizer_desc.DepthBiasClamp = 0.0f;
		d3d_rasterizer_desc.DepthClipEnable = TRUE;
		d3d_rasterizer_desc.ScissorEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		d3d_rasterizer_desc.AntialiasedLineEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		HRESULT res_d3d_device_create_rasterizer_state = d3d_device->CreateRasterizerState(&d3d_rasterizer_desc, &m_rect_light_rs);
		assert(SUCCEEDED(res_d3d_device_create_rasterizer_state));
	}

	m_rect_light_uniform_buffer_per_frame_binding = NULL;
	{
		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(rect_light_uniform_buffer_per_frame_binding_t);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, NULL, &m_rect_light_uniform_buffer_per_frame_binding);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	m_post_process_vao = NULL;

	m_post_process_vs = NULL;
	{
		HRESULT res_d3d_device_create_vertex_shader = d3d_device->CreateVertexShader(post_process_vs_bytecode, sizeof(post_process_vs_bytecode), NULL, &m_post_process_vs);
		assert(SUCCEEDED(res_d3d_device_create_vertex_shader));
	}

	m_post_process_fs = NULL;
	{
		HRESULT res_d3d_device_create_pixel_shader = d3d_device->CreatePixelShader(post_process_fs_bytecode, sizeof(post_process_fs_bytecode), NULL, &m_post_process_fs);
		assert(SUCCEEDED(res_d3d_device_create_pixel_shader));
	}

	m_post_process_rs = NULL;
	{
		D3D11_RASTERIZER_DESC d3d_rasterizer_desc;
		d3d_rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		d3d_rasterizer_desc.CullMode = D3D11_CULL_BACK;
		d3d_rasterizer_desc.FrontCounterClockwise = FALSE;
		d3d_rasterizer_desc.DepthBias = 0;
		d3d_rasterizer_desc.SlopeScaledDepthBias = 0.0f;
		d3d_rasterizer_desc.DepthBiasClamp = 0.0f;
		d3d_rasterizer_desc.DepthClipEnable = TRUE;
		d3d_rasterizer_desc.ScissorEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		d3d_rasterizer_desc.AntialiasedLineEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		HRESULT res_d3d_device_create_rasterizer_state = d3d_device->CreateRasterizerState(&d3d_rasterizer_desc, &m_post_process_rs);
		assert(SUCCEEDED(res_d3d_device_create_rasterizer_state));
	}

	m_ltc_lut_sampler = NULL;
	{
		D3D11_SAMPLER_DESC d3d_sampler_desc;
		d3d_sampler_desc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		d3d_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.MipLODBias = 0.0;
		d3d_sampler_desc.MaxAnisotropy = 0U;
		d3d_sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		d3d_sampler_desc.BorderColor[0] = 0.0;
		d3d_sampler_desc.BorderColor[1] = 0.0;
		d3d_sampler_desc.BorderColor[2] = 0.0;
		d3d_sampler_desc.BorderColor[3] = 1.0;
		d3d_sampler_desc.MinLOD = 0.0;
		d3d_sampler_desc.MaxLOD = 4096.0;

		HRESULT res_d3d_device_create_sampler = d3d_device->CreateSamplerState(&d3d_sampler_desc, &m_ltc_lut_sampler);
		assert(SUCCEEDED(res_d3d_device_create_sampler));
	}

	m_ltc_matrix_lut = NULL;
	{
		static_assert((4U * 64U * 64U) == (sizeof(g_ltc_matrix_lut_tr_data) / sizeof(g_ltc_matrix_lut_tr_data[0])), "");

		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = 64U;
		d3d_texture2d_desc.Height = 64U;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = 1U;
		d3d_texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		int8_t ltc_ggx_matrix_unorm_data[4 * 64 * 64];
		for (int i = 0; i < (4 * 64 * 64); ++i)
		{
			ltc_ggx_matrix_unorm_data[i] = float_to_snorm(g_ltc_matrix_lut_tr_data[i]);
		}

		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = ltc_ggx_matrix_unorm_data;
		d3d_subresource_data.SysMemPitch = sizeof(int8_t) * 4 * 64;
		d3d_subresource_data.SysMemSlicePitch = sizeof(int8_t) * 4 * 64 * 64;

		HRESULT res_d3d_device_create_texture = d3d_device->CreateTexture2D(&d3d_texture2d_desc, &d3d_subresource_data, &m_ltc_matrix_lut);
		assert(SUCCEEDED(res_d3d_device_create_texture));
	}

	m_ltc_matrix_lut_srv = NULL;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC d3d_shader_resource_view_desc;
		d3d_shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
		d3d_shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		d3d_shader_resource_view_desc.Texture2DArray.MostDetailedMip = 0U;
		d3d_shader_resource_view_desc.Texture2DArray.MipLevels = 1U;
		d3d_shader_resource_view_desc.Texture2DArray.FirstArraySlice = 0U;
		d3d_shader_resource_view_desc.Texture2DArray.ArraySize = 1U;

		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateShaderResourceView(m_ltc_matrix_lut, &d3d_shader_resource_view_desc, &m_ltc_matrix_lut_srv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	m_ltc_norm_lut = NULL;
	{
		// UE: [InitializeFeatureLevelDependentTextures] (https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/Renderer/Private/SystemTextures.cpp#L278)

		// The PreintegratedGF maybe used on forward shading inluding mobile platorm, intialize it anyway.

		// for testing, with 128x128 R8G8 we are very close to the reference (if lower res is needed we might have to add an offset to counter the 0.5f texel shift)
		constexpr bool const bReference = false;

		// for low roughness we would get banding with PF_R8G8 but for low spec it could be used, for now we don't do this optimization
		DXGI_FORMAT Format = DXGI_FORMAT_R16G16_UNORM;

		constexpr int32_t const Width = 128;
		constexpr int32_t const Height = bReference ? 128 : 32;

		// Write the contents of the texture.
		uint16_t DestBuffer[2U * Width * Height];
		constexpr int32_t const DestStride = 2U * Width;

		// x is NoV, y is roughness
		for (int32_t y = 0; y < Height; ++y)
		{
			float Roughness = (float)(y + 0.5f) / Height;
			float m = Roughness * Roughness;
			float m2 = m * m;

			for (int32_t x = 0; x < Width; ++x)
			{
				float NoV = (float)(x + 0.5f) / Width;

				// tangent space where N is (0, 0, 1)
				// since the TR BRDF is isotropic, the outgoing direction V is assumed to be in the XOZ plane,
				DirectX::XMFLOAT3 V;
				V.x = std::sqrt(1.0f * 1.0f - NoV * NoV); // sin
				V.y = 0.0f;
				V.z = NoV; // cos

				float A = 0.0f;
				float B = 0.0f;
				float C = 0.0f;

				const uint32_t NumSamples = 128;
				for (uint32_t i = 0; i < NumSamples; i++)
				{
					float E1 = (float)i / NumSamples;
					float E2 = (double)ReverseBits(i) / (double)0x100000000LL;

					{
						float Phi = 2.0f * DirectX::XM_PI * E1;

						float CosPhi = std::cos(Phi);
						float SinPhi = std::sin(Phi);
						float CosTheta = std::sqrt((1.0f - E2) / (1.0f + (m2 - 1.0f) * E2));
						float SinTheta = std::sqrt(1.0f - CosTheta * CosTheta);

						DirectX::XMFLOAT3 H(SinTheta * std::cos(Phi), SinTheta * std::sin(Phi), CosTheta);

						// L = 2.0f * dot(V, H) * H - V;
						DirectX::XMFLOAT3 L;
						{
							DirectX::XMVECTOR SIMD_V = DirectX::XMLoadFloat3(&V);
							DirectX::XMVECTOR SIMD_H = DirectX::XMLoadFloat3(&H);
							DirectX::XMStoreFloat3(&L, DirectX::XMVectorSubtract(DirectX::XMVectorMultiply(DirectX::XMVectorScale(DirectX::XMVector3Dot(SIMD_V, SIMD_H), 2.0f), SIMD_H), SIMD_V));
						}

						float NoL = std::max(L.z, 0.0f);
						float NoH = std::max(H.z, 0.0f);
						float VoH;
						{
							DirectX::XMVECTOR SIMD_V = DirectX::XMLoadFloat3(&V);
							DirectX::XMVECTOR SIMD_H = DirectX::XMLoadFloat3(&H);
							VoH = DirectX::XMVectorGetX(DirectX::XMVector3Dot(SIMD_V, SIMD_H));
						}
						VoH = std::max(VoH, 0.0f);

						if (NoL > 0.0f)
						{
							float Vis_SmithV = NoL * (NoV * (1 - m) + m);
							float Vis_SmithL = NoV * (NoL * (1 - m) + m);
							float Vis = 0.5f / (Vis_SmithV + Vis_SmithL);

							float NoL_Vis_PDF = NoL * Vis * (4.0f * VoH / NoH);
							float Fc = 1.0f - VoH;
							Fc *= ((Fc * Fc) * (Fc * Fc));
							A += NoL_Vis_PDF * (1.0f - Fc);
							B += NoL_Vis_PDF * Fc;
						}
					}
				}
				A /= NumSamples;
				B /= NumSamples;

				DestBuffer[DestStride * y + 2 * x] = (int32_t)(std::min(std::max(A, 0.0f), 1.0f) * 65535.0f + 0.5f);
				DestBuffer[DestStride * y + 2 * x + 1] = (int32_t)(std::min(std::max(B, 0.0f), 1.0f) * 65535.0f + 0.5f);
			}
		}

		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = Width;
		d3d_texture2d_desc.Height = Height;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = 1U;
		d3d_texture2d_desc.Format = Format;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		static_assert(sizeof(DestBuffer) == (sizeof(uint16_t) * DestStride * Height), "");
		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = DestBuffer;
		d3d_subresource_data.SysMemPitch = sizeof(uint16_t) * DestStride;
		d3d_subresource_data.SysMemSlicePitch = sizeof(uint16_t) * DestStride * Height;

		HRESULT res_d3d_device_create_texture = d3d_device->CreateTexture2D(&d3d_texture2d_desc, &d3d_subresource_data, &m_ltc_norm_lut);
		assert(SUCCEEDED(res_d3d_device_create_texture));
	}

	m_ltc_norm_lut_srv = NULL;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC d3d_shader_resource_view_desc;
		d3d_shader_resource_view_desc.Format = DXGI_FORMAT_R16G16_UNORM;
		d3d_shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		d3d_shader_resource_view_desc.Texture2DArray.MostDetailedMip = 0U;
		d3d_shader_resource_view_desc.Texture2DArray.MipLevels = 1U;
		d3d_shader_resource_view_desc.Texture2DArray.FirstArraySlice = 0U;
		d3d_shader_resource_view_desc.Texture2DArray.ArraySize = 1U;

		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateShaderResourceView(m_ltc_norm_lut, &d3d_shader_resource_view_desc, &m_ltc_norm_lut_srv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	m_attachment_backup_odd = NULL;
	{
		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = g_resolution_width;
		d3d_texture2d_desc.Height = g_resolution_height;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = 1U;
		d3d_texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateTexture2D(&d3d_texture2d_desc, NULL, &m_attachment_backup_odd);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	m_attachment_backup_odd_rtv = NULL;
	{
		D3D11_RENDER_TARGET_VIEW_DESC d3d_render_target_view_desc;
		d3d_render_target_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3d_render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		d3d_render_target_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_d3d_device_create_render_target_view = d3d_device->CreateRenderTargetView(m_attachment_backup_odd, &d3d_render_target_view_desc, &m_attachment_backup_odd_rtv);
		assert(SUCCEEDED(res_d3d_device_create_render_target_view));
	}

	m_attachment_backup_odd_srv = NULL;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC d3d_shader_resource_view_desc;
		d3d_shader_resource_view_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3d_shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		d3d_shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		d3d_shader_resource_view_desc.Texture2D.MipLevels = 1U;

		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateShaderResourceView(m_attachment_backup_odd, &d3d_shader_resource_view_desc, &m_attachment_backup_odd_srv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	m_attachment_depth = NULL;
	{
		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = g_resolution_width;
		d3d_texture2d_desc.Height = g_resolution_width;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = 1U;
		d3d_texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateTexture2D(&d3d_texture2d_desc, NULL, &m_attachment_depth);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	m_attachment_depth_dsv = NULL;
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC d3d_depth_stencil_view_desc;
		d3d_depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		d3d_depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		d3d_depth_stencil_view_desc.Flags = 0U;
		d3d_depth_stencil_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateDepthStencilView(m_attachment_depth, &d3d_depth_stencil_view_desc, &m_attachment_depth_dsv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	g_camera_controller.m_eye_position = DirectX::XMFLOAT3(0.00000000, 6.00000000, -0.500000000);
	g_camera_controller.m_eye_direction = DirectX::XMFLOAT3(0.00000000, 0.174311504, 1.99238944);
	g_camera_controller.m_up_direction = DirectX::XMFLOAT3(0.0, 1.0, 0.0);
}

void Demo::Tick(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, IDXGISwapChain *dxgi_swap_chain)
{
	// Upload
	plane_uniform_buffer_per_frame_binding_t plane_uniform_buffer_data_per_frame_binding;
	rect_light_uniform_buffer_per_frame_binding_t rect_light_uniform_buffer_data_per_frame_binding;
	{
		// camera
		{
			DirectX::XMFLOAT3 eye_position = g_camera_controller.m_eye_position;
			DirectX::XMFLOAT3 eye_direction = g_camera_controller.m_eye_direction;
			DirectX::XMFLOAT3 up_direction = g_camera_controller.m_up_direction;

			DirectX::XMMATRIX tmp_view_transform = DirectX::XMMatrixLookToRH(DirectX::XMLoadFloat3(&eye_position), DirectX::XMLoadFloat3(&eye_direction), DirectX::XMLoadFloat3(&up_direction));
			DirectX::XMFLOAT4X4 view_transform;
			DirectX::XMStoreFloat4x4(&view_transform, tmp_view_transform);

			float fov_angle_y = 2.0 * atan((1.0 / 2.0));
			DirectX::XMMATRIX tmp_projection_transform = DirectX::XMMatrixPerspectiveFovRH(fov_angle_y, 1.0, 7.0, 7777.0);
			DirectX::XMFLOAT4X4 projection_transform;
			DirectX::XMStoreFloat4x4(&projection_transform, tmp_projection_transform);

			plane_uniform_buffer_data_per_frame_binding.view_transform = view_transform;
			plane_uniform_buffer_data_per_frame_binding.projection_transform = projection_transform;
			plane_uniform_buffer_data_per_frame_binding.eye_position = eye_position;

			rect_light_uniform_buffer_data_per_frame_binding.view_transform = view_transform;
			rect_light_uniform_buffer_data_per_frame_binding.projection_transform = projection_transform;
		}

		// light
		{
			plane_uniform_buffer_data_per_frame_binding.rect_light_vetices[0] = DirectX::XMFLOAT4(-4.0, 2.0, 32.0, 1.0);
			plane_uniform_buffer_data_per_frame_binding.rect_light_vetices[1] = DirectX::XMFLOAT4(4.0, 2.0, 32.0, 1.0);
			plane_uniform_buffer_data_per_frame_binding.rect_light_vetices[2] = DirectX::XMFLOAT4(4.0, 10.0, 32.0, 1.0);
			plane_uniform_buffer_data_per_frame_binding.rect_light_vetices[3] = DirectX::XMFLOAT4(-4.0, 10.0, 32.0, 1.0);
			plane_uniform_buffer_data_per_frame_binding.culling_range = 20.0;
			plane_uniform_buffer_data_per_frame_binding.intensity = 4.0;

			rect_light_uniform_buffer_data_per_frame_binding.rect_light_vetices[0] = DirectX::XMFLOAT4(-4.0, 2.0, 32.0, 1.0);
			rect_light_uniform_buffer_data_per_frame_binding.rect_light_vetices[1] = DirectX::XMFLOAT4(4.0, 2.0, 32.0, 1.0);
			rect_light_uniform_buffer_data_per_frame_binding.rect_light_vetices[2] = DirectX::XMFLOAT4(-4.0, 10.0, 32.0, 1.0);
			rect_light_uniform_buffer_data_per_frame_binding.rect_light_vetices[3] = DirectX::XMFLOAT4(4.0, 10.0, 32.0, 1.0);
			rect_light_uniform_buffer_data_per_frame_binding.intensity = 4.0;
		}

		// mesh
		{
			DirectX::XMFLOAT4X4 model_transform;
			DirectX::XMStoreFloat4x4(&model_transform, DirectX::XMMatrixIdentity());
			plane_uniform_buffer_data_per_frame_binding.model_transform = model_transform;
			plane_uniform_buffer_data_per_frame_binding.dcolor = DirectX::XMFLOAT3(1.0, 1.0, 1.0);
			plane_uniform_buffer_data_per_frame_binding.scolor = DirectX::XMFLOAT3(0.23, 0.23, 0.23);
			plane_uniform_buffer_data_per_frame_binding.roughness = 0.25;
		}
	}
	d3d_device_context->UpdateSubresource(m_plane_uniform_buffer_per_frame_binding, 0U, NULL, &plane_uniform_buffer_data_per_frame_binding, sizeof(plane_uniform_buffer_per_frame_binding_t), sizeof(plane_uniform_buffer_per_frame_binding_t));
	d3d_device_context->UpdateSubresource(m_rect_light_uniform_buffer_per_frame_binding, 0U, NULL, &rect_light_uniform_buffer_data_per_frame_binding, sizeof(rect_light_uniform_buffer_per_frame_binding_t), sizeof(rect_light_uniform_buffer_per_frame_binding_t));

	// Light Pass
	{
		d3d_device_context->OMSetRenderTargets(1U, &m_attachment_backup_odd_rtv, m_attachment_depth_dsv);

		FLOAT color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		d3d_device_context->ClearRenderTargetView(m_attachment_backup_odd_rtv, color);

		FLOAT depth = 1.0f;
		d3d_device_context->ClearDepthStencilView(m_attachment_depth_dsv, D3D11_CLEAR_DEPTH, depth, 0U);

		// glEnable(GL_DEPTH_TEST);
		// glDepthFunc(GL_LESS);
		// glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		D3D11_VIEWPORT viewport = {0.0f, 0.0f, g_resolution_width, g_resolution_height, 0.0f, 1.0f};
		d3d_device_context->RSSetViewports(1U, &viewport);

		// Draw Plane
		{
			d3d_device_context->RSSetState(m_plane_rs);

			d3d_device_context->VSSetShader(m_plane_vs, NULL, 0U);
			d3d_device_context->PSSetShader(m_plane_fs, NULL, 0U);

			d3d_device_context->VSSetConstantBuffers(0U, 1U, &m_plane_uniform_buffer_per_frame_binding);
			d3d_device_context->PSSetConstantBuffers(0U, 1U, &m_plane_uniform_buffer_per_frame_binding);

			d3d_device_context->PSSetSamplers(0U, 1U, &m_ltc_lut_sampler);
			d3d_device_context->PSSetShaderResources(0U, 1U, &m_ltc_matrix_lut_srv);
			d3d_device_context->PSSetShaderResources(1U, 1U, &m_ltc_norm_lut_srv);

			d3d_device_context->IASetInputLayout(m_plane_vao);

			ID3D11Buffer *vertex_buffers[2] = {m_plane_vb_position, m_plane_vb_varying};
			UINT strides[2] = {sizeof(float) * 3, sizeof(float) * 3};
			UINT offsets[2] = {0U, 0U};
			d3d_device_context->IASetVertexBuffers(0U, 2U, vertex_buffers, strides, offsets);
			d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			d3d_device_context->DrawInstanced(4U, 1U, 0U, 0U);
		}

		// Draw Rect Light
		{
			d3d_device_context->RSSetState(m_rect_light_rs);

			d3d_device_context->VSSetShader(m_rect_light_vs, NULL, 0U);
			d3d_device_context->PSSetShader(m_rect_light_fs, NULL, 0U);

			d3d_device_context->VSSetConstantBuffers(0U, 1U, &m_rect_light_uniform_buffer_per_frame_binding);
			d3d_device_context->PSSetConstantBuffers(0U, 1U, &m_rect_light_uniform_buffer_per_frame_binding);

			d3d_device_context->IASetInputLayout(m_rect_light_vao);
			d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			d3d_device_context->DrawInstanced(4U, 1U, 0U, 0U);
		}
	}

	// Post Process Pass
	{
		d3d_device_context->OMSetRenderTargets(1U, &m_attachment_backbuffer_rtv, NULL);

		d3d_device_context->RSSetState(m_post_process_rs);

		// glDisable(GL_DEPTH_TEST);

		d3d_device_context->VSSetShader(m_post_process_vs, NULL, 0U);
		d3d_device_context->PSSetShader(m_post_process_fs, NULL, 0U);

		d3d_device_context->PSSetSamplers(0U, 1U, &m_ltc_lut_sampler);
		d3d_device_context->PSSetShaderResources(0U, 1U, &m_attachment_backup_odd_srv);

		d3d_device_context->IASetInputLayout(m_post_process_vao);
		d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		d3d_device_context->DrawInstanced(3U, 1U, 0U, 0U);

		// unbind
		ID3D11ShaderResourceView *shader_resource_views = {NULL};
		d3d_device_context->PSSetShaderResources(0U, 1U, &shader_resource_views);
	}

	HRESULT res_dxgi_swap_chain_present = dxgi_swap_chain->Present(1U, 0U);
	assert(SUCCEEDED(res_dxgi_swap_chain_present));
}

static int8_t float_to_snorm(float unpacked_input)
{
	// UE: [FPackedNormal](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/RenderCore/Public/PackedNormal.h#L98)

	float scale_float = unpacked_input * static_cast<float>(INT8_MAX);
	int32_t round_to_int = (static_cast<int32_t>(scale_float + scale_float + 0.5f) >> 1);
	int32_t clamp = std::max(static_cast<int32_t>(INT8_MIN), std::min(static_cast<int32_t>(INT8_MAX), round_to_int));
	return ((int8_t)clamp);
}