#version 460 core
precision highp float;

out VSOut
{
    vec2 texCoord;
}vsout;

void main() 
{
    float x = float((gl_VertexID & 1) << 2);
    float y = float((gl_VertexID & 2) << 1);
    vsout.texCoord = vec2(x * 0.5f, y * 0.5f);
    gl_Position = vec4(x - 1.0, y - 1.0, 0.f, 1.f);
}