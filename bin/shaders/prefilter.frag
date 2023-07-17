#version 460 core
#define PI 3.1415926535897932384626433832795
precision highp float; 

layout(location = 0) out vec4 fragColor4fv;					// for diffuses/spec pre filter maps
layout(location = 1) out vec2 fragColor2fv;					// for BRDF integration map

layout(binding = 0) uniform sampler2D texHDR;

//fbo
layout(location = 0) uniform int width;
layout(location = 1) uniform int height;
layout(location = 2) uniform int samples;
layout(location = 3) uniform float mipmapLevel;
layout(location = 4) uniform float roughness;			

in VS_OUT
{
	vec2 texCoord;
}fsIn;

//convert texcoord to pixels
float t2p(float t, int nPixels)
{
	return t * float(nPixels) - 0.5f;
}	

vec3 random_pcg3d(uvec3 v) {
	v = v * 1664525u + 1013904223u;
	v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
	v ^= v >> 16u;
	v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
	return vec3(v) * (1.0/float(0xffffffffu));
}

float radicalInverse(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; 
}

vec2 hammersley(uint n, uint N) {
  return vec2(float(n) / float(N), radicalInverse(n));
}

float halton(uint base, uint index) {
	float result = 0.0;
	float digitWeight = 1.0;
	while (index > 0u)
	{
		digitWeight = digitWeight / float(base);
		uint nominator = index % base;										//compute the remainder with the modulo operation
		result += float(nominator) * digitWeight;
		index = index / base; 
	}
	return result;
}

vec3 sphericalEnvMapToDirection(vec2 texCoord)
{
	float theta = PI * (1.f - texCoord.t);
	float phi = 2.f * PI * (0.5f - texCoord.s);
	return vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

vec2 directionToSphericalEnvmap(vec3 dir) 
{
	float phi = atan(dir.y, dir.x);
	float theta = acos(dir.z);
	float s = 0.5 - phi / (2.0 * PI);
	float t = 1.0 - theta / PI;
	return vec2(s, t);
}

mat3 getNormalFrame(vec3 normal)
{
	vec3 someVec = vec3(1.0, 0.0, 0.0);
	float dd = dot(someVec, normal);	
	vec3 tangent = vec3(0.0, 1.0, 0.0);
	if(1.0 - abs(dd) > 1e-6) 
		tangent = normalize(cross(someVec, normal));
	
	vec3 bitangent = cross(normal, tangent);
	return mat3(tangent, bitangent, normal);
}

float G1_GGX_Schlick(float NoV, float roughness)
{
	float k = (roughness * roughness) / 2.f;
	return max(NoV, 0.001f) / (NoV * (1.f - k) + k);
}

float G_Smith(float NoV, float NoL, float roughness)
{
	return G1_GGX_Schlick(NoL, roughness) * G1_GGX_Schlick(NoV, roughness);
}

subroutine void PreFilterPass();

layout(index = 0)
subroutine(PreFilterPass)
void prefilterEnvMapDiffuse()
{
	float px = t2p(fsIn.texCoord.x, width);
	float py = t2p(fsIn.texCoord.y, height);
	vec3 normal = sphericalEnvMapToDirection(fsIn.texCoord);

	mat3 normalTransform = getNormalFrame(normal);
	vec3 result = vec3(0.0);
	uint N = uint(samples);
	for(uint n = 0u; n < N; n++)
	{
		vec3 random = random_pcg3d(uvec3(px, py, n));
		float phi = 2.0 * PI * random.x;
		float theta = asin(sqrt(random.y));
		vec3 posLocal = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
		vec3 posWorld = normalTransform * posLocal;
		vec2 uv = directionToSphericalEnvmap(posWorld);
		vec3 radiance = textureLod(texHDR, uv, mipmapLevel).rgb;			
		result += radiance;
	}

	fragColor4fv = vec4(result / float(N), 1.f);
}

layout(index = 1)
subroutine(PreFilterPass)
void prefilterEnvMapSpecular()
{
	if(mipmapLevel > 0.f)
	{
		float px = t2p(fsIn.texCoord.x, width);
		float py = t2p(fsIn.texCoord.y, height);
		vec3 normal = sphericalEnvMapToDirection(fsIn.texCoord);

		mat3 normalTransform = getNormalFrame(normal);
		vec3 v = normal;
		vec3 result = vec3(0.0);
		float totalWeight = 0.0;
		uint N = uint(samples);
		for(uint n = 1u; n <= N; n++) 
		{
			vec2 random = vec2(halton(2u, n), halton(3u, n));
			float phi = 2.0 * PI * random.x;
			float u = random.y;
			float alpha = roughness * roughness;
			float theta = acos(sqrt((1.0 - u) / (1.0 + (alpha * alpha - 1.0) * u)));
			vec3 posLocal = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 h = normalTransform * posLocal;
			vec3 l = 2.0 * dot(v, h) * h - v; 
			float NoL = dot(normal, l);
			if(NoL > 0.0)
			{
				vec2 uv = directionToSphericalEnvmap(l);
				vec3 radiance = textureLod(texHDR, uv, mipmapLevel).rgb;
				result += radiance * NoL;
				totalWeight += NoL;
			}
		}

		fragColor4fv = vec4(result / totalWeight, 1.f);
	}
	else
		fragColor4fv = textureLod(texHDR, fsIn.texCoord, 0.f);									//copy image directly for 0th mipmaplevel
}


layout(index = 2)
subroutine(PreFilterPass)
void integrateBRDF()
{
	float r = fsIn.texCoord.y;
	float NoV = fsIn.texCoord.x;
	float px = t2p(fsIn.texCoord.x, width);
	float py = t2p(fsIn.texCoord.y, height);

	//view direction in normal space from spherical coordinates
	float thetaView = acos(NoV);
	vec3 v = vec3(sin(thetaView), 0.0, cos(thetaView));									
  
	vec2 result = vec2(0.0);
	uint N = uint(samples);
	for(uint n = 0u; n < N; n++)
	{
		vec3 random = random_pcg3d(uvec3(px, py, n));
		float phi = 2.f * PI * random.x;
		float u = random.y;
		float alpha = r * r;
		float theta = acos(sqrt((1.f - u) / (1.f + (alpha * alpha - 1.f) * u)));
		vec3 h = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
		vec3 l = 2.f * dot(v, h) * h - v;															// or use L = reflect(-V, H);
		float NoL = clamp(l.z, 0.f, 1.f);
		float NoH = clamp(h.z, 0.f, 1.f);
		float VoH = clamp(dot(v, h), 0.0, 1.0);
		if(NoL > 0.0) 
		{
			float G = G_Smith(NoV, NoL, r);
			float G_Vis = G * VoH / (NoH * NoV);
			float Fc = pow(1.0 - VoH, 5.0);
			result.x += (1.0 - Fc) * G_Vis;
			result.y += Fc * G_Vis;
		}
	}

	fragColor2fv = result / float(N);
}

subroutine uniform PreFilterPass preFilterPass;

void main()
{
	preFilterPass();
}