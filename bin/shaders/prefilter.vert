#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoord;

out VS_OUT
{	
	vec2 texCoord;
}vsOut;

void main()
{
	vsOut.texCoord = aTexCoord;
	gl_Position = vec4(aPos, 1.f);
}