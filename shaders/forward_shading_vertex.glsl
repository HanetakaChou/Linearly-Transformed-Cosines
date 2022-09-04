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

layout(location = 0) in highp vec3 vertex_input_position;
layout(location = 1) in highp vec2 vertex_input_uv;

layout(location = 0) out highp vec2 vertex_output_uv;

void main()
{
    gl_Position = P * V * M * vec4(vertex_input_position, 1.0f);
    vertex_output_uv = vertex_input_uv;
}
