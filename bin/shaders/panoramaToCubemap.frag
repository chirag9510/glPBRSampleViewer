#version 460 core
#define PI 3.1415926535897932384626433832795
precision highp float;

out vec4 fragColor;

layout(binding = 0) uniform sampler2D texPanorama;

layout(location = 0) uniform int face;

in VSOut
{
	vec2 texCoord;
}fsin;

vec3 uvToXYZ(int face, vec2 uv)
{
	if(face == 0)
		return vec3(1.f, uv.y, -uv.x);
	else if(face == 1)
		return vec3(-1.f, uv.y, uv.x);
	else if(face == 2)
		return vec3(uv.x, -1.f, uv.y);
	else if(face == 3)
		return vec3(uv.x, 1.f, -uv.y);
	else if(face == 4)
		return vec3(uv.x, uv.y, 1.f);
	else 
		return vec3(-uv.x, uv.y, -1.f);
}

vec2 dirToUV(vec3 dir)
{
	return vec2(
		0.5f + 0.5f * atan(dir.z, dir.x) / PI,
		1.f - acos(-dir.y) / PI);													//keep the image vertically flipped					
	//	1.f - acos(dir.y) / PI);
}

void main()
{
	vec2 texCoord = fsin.texCoord * 2.0 - 1.0;
	vec3 scan = uvToXYZ(face, texCoord);
	vec3 direction = normalize(scan);
	vec2 src = dirToUV(direction);

	fragColor = vec4(texture(texPanorama, src).rgb, 1.f);
}