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
#extension GL_EXT_control_flow_attributes : enable

#include "unified_pipeline_layout.h"
#include "ltc.glsli"
#include "preintegrated_hdr.glsli"

layout(location = 0) in highp vec3 interpolated_position;
layout(location = 1) in highp vec3 interpolated_normal;

layout(location = 0) out highp vec4 fragment_output_color;

highp vec3 ToLinear(highp vec3 v);

void main()
{
    highp vec3 output_color = vec3(0.0, 0.0, 0.0);

    highp vec3 points[4] = vec3[4](rect_light_vetices[0].xyz, rect_light_vetices[1].xyz, rect_light_vetices[2].xyz, rect_light_vetices[3].xyz);
    highp vec3 light_color = vec3(intensity, intensity, intensity);

    highp vec3 P = interpolated_position;
    highp vec3 N = interpolated_normal;
    highp vec3 V = normalize(eye_position - P);
    highp vec3 diffuse_color = ToLinear(dcolor);
    highp vec3 specular_color = ToLinear(scolor);

    highp float light_attenutation = evaluate_ltc_light_attenuation(culling_range, P, points);
    [[dont_flatten]]
    if (light_attenutation > 0.0)
    {
        output_color += light_color * light_attenutation * evaluate_brdf_ltc(diffuse_color, roughness, specular_color, P, N, V, points);
    }

    fragment_output_color = vec4(output_color, 1.0);
}

highp vec3 PowVec3(highp vec3 v, highp float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

highp vec3 ToLinear(highp vec3 v)
{
    const highp float gamma = 2.2;
    return PowVec3(v, gamma);
}