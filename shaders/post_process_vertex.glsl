#version 310 es

void main()
{
    const highp vec2 positions[3] = vec2[3](vec2(-1.0, -3.0), vec2(-1.0, 1.0), vec2(3.0, 1.0));

    gl_Position = vec4(positions[gl_VertexIndex], 0.5, 1.0);
}