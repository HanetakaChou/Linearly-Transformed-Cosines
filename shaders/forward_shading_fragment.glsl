#version 310 es

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

#extension GL_GOOGLE_include_directive : enable

#include "forward_shading_pipeline_layout.h"

layout(location = 0) in highp vec2 interpolated_uv;

layout(location = 0) out highp vec4 fragment_output_color;

void main()
{
   fragment_output_color = texture(_material_set_emissive_texture_binding, interpolated_uv);
}