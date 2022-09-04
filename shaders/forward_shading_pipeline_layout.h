//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _FORWARD_SHADING_PIPELINE_LAYOUT_H_
#define _FORWARD_SHADING_PIPELINE_LAYOUT_H_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct forward_shading_layout_global_set_frame_uniform_buffer_binding
{
    DirectX::XMFLOAT4X4 view_transform;
    DirectX::XMFLOAT4X4 projection_transform;
};

struct forward_shading_layout_global_set_object_uniform_buffer_binding
{
    DirectX::XMFLOAT4X4 model_transform;
};

#elif defined(GL_SPIRV) || defined(VULKAN)

layout(set = 0, binding = 0, column_major) uniform _global_set_frame_binding
{
    highp mat4x4 V;
    highp mat4x4 P;
};

layout(set = 0, binding = 1, column_major) uniform _global_set_object_binding
{
    highp mat4x4 M;
};

layout(set = 1, binding = 0) highp uniform sampler2D _material_set_emissive_texture_binding;

#else
#error Unknown Compiler
#endif

#endif