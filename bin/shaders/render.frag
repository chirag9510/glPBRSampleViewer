#version 460 core
precision highp float; 
precision highp int;
#define PI 3.1415926535897932384626433832795
#define RECIPROCAL_PI (1.f / PI)

out vec4 fragColor;

layout(binding = 0) uniform sampler2DArray texMatArray;
layout(binding = 1) uniform sampler2D texEnvMapDiffuse;
layout(binding = 2) uniform sampler2D texEnvMapSpecular;
layout(binding = 3) uniform sampler2D texBRDFIntegration;
layout(binding = 4) uniform sampler2D texAO;
layout(binding = 5) uniform samplerCube texCubemap;

layout(location = 0) uniform float reflectance;
layout(location = 1) uniform float mipmapCount = 6.f;
layout(location = 2) uniform bool emissive;
layout(location = 3) uniform bool seperateAO;
layout(location = 4) uniform bool blended;
layout(location = 5) uniform bool alphaDiffuse;							//alpha value is in the diffuse texture as 4th component, else alpha value is in the AI_MATKEY_OPACITY assimp key from the mesh
layout(location = 6) uniform float opacity;

layout(location = 7) uniform bool enableIBL;
layout(location = 8) uniform bool enableAO;
layout(location = 9) uniform vec3 lightDir1;
layout(location = 10) uniform float lightIntensity1;
layout(location = 11) uniform vec3 lightDir2;
layout(location = 12) uniform float lightIntensity2;
layout(location = 13) uniform vec3 lightColor1;
layout(location = 14) uniform vec3 lightColor2;

in VSOut
{
    vec3 viewDir;
	vec2 texCoord;
	mat3 TBN;
	
	//skybox only
	vec3 texCoordCube;
}fsin;


vec3 lin2rgb(vec3 lin)
{
	return pow(lin, vec3(1.f / 2.2f));
}
vec3 rgb2lin(vec3 rgb)
{
	return pow(rgb, vec3(2.2f));
}

subroutine void RenderPass();

//skybox
layout(index = 0)
subroutine(RenderPass)
void renderSkybox()
{
	//stbi_loadf already loads in linear space, gamma correct to srgb 
	fragColor = vec4(lin2rgb(texture(texCubemap, fsin.texCoordCube).rgb), 1.f);
}

//Default rendering pass
vec2 directionToSphericalEnvmap(vec3 dir) 
{
	float phi = atan(dir.z, dir.x);
	float theta = acos(dir.y);
	float s = 0.5 - phi / (2.0 * PI);
	float t = 1.0 - theta / PI;
	return vec2(s, t);
}

vec3 specularIBL(vec3 f0, float roughness, vec3 n, vec3 v)
{
	float NoV = clamp(dot(n, v), 0.f, 1.f);
	vec3 r = reflect(-v, n);
	vec2 uv = directionToSphericalEnvmap(r);
	vec3 t1 = textureLod(texEnvMapSpecular, uv, roughness * mipmapCount).rgb; 
	vec4 brdfIntegration = texture(texBRDFIntegration, vec2(NoV, roughness));
	vec3 t2 = vec3(f0 * brdfIntegration.x + brdfIntegration.y);
	return t1 * t2;
}

vec3 fresnelSchlick(float cosTheta, vec3 f0)
{
	return f0 + (1.f - f0) * pow(1.f - cosTheta, 5.f);
}

float D_GGX(float NoH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float NoH2 = NoH * NoH;
	float b = (NoH2 * (alpha2 - 1.f) + 1.f);
	return alpha2 * RECIPROCAL_PI / (b * b);
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

vec3 brdfMicrofacet(in vec3 l, in vec3 v, in vec3 n, in float metallic, in float roughness, in vec3 baseColor)	
{
	vec3 h = normalize(v + l);
	float NoV = clamp(dot(n, v), 0.f, 1.f);
	float NoL = clamp(dot(n, l), 0.f, 1.f);
	float NoH = clamp(dot(n, h), 0.f, 1.f);
	float VoH = clamp(dot(v, h), 0.f, 1.f);

	vec3 f0 = vec3(0.16 * reflectance * reflectance);
	f0 = mix(f0, baseColor, metallic);

	vec3 F = fresnelSchlick(VoH, f0);
	float D = D_GGX(NoH, roughness);
	float G = G_Smith(NoV, NoL, roughness);
	vec3 spec = (F * D * G) / (4.f *  max(NoV, 0.001) * max(NoL, 0.001));					//cant be 0 or -ve

	vec3 diff = (baseColor * (vec3(1.f) - F) * (1.f - metallic)) * RECIPROCAL_PI;

	return diff + spec;
}


layout(index = 1)
subroutine(RenderPass)
void renderDefault()
{
	vec3 baseColor = rgb2lin(texture(texMatArray, vec3(fsin.texCoord, 0.f)).rgb);
	float roughness = texture(texMatArray, vec3(fsin.texCoord, 2.f)).g;
	float metallic = texture(texMatArray, vec3(fsin.texCoord, 2.f)).b;
	vec3 n = normalize(fsin.TBN * (texture(texMatArray, vec3(fsin.texCoord, 1.f)).rgb * 2.0  - 1.0));
	vec3 v = normalize(fsin.viewDir);

	vec3 radiance = vec3(0.f);
	if(emissive)
		radiance = rgb2lin(texture(texMatArray, vec3(fsin.texCoord, 3.f)).rgb);

	if(enableIBL)
	{
		vec3 f0 = vec3(0.16 * (reflectance * reflectance));
		f0 = mix(f0, baseColor, metallic);
  
		//IBL diffuse
		vec3 rhoD = baseColor * (1.0 - metallic) * (vec3(1.0) - f0);
		radiance += rhoD * texture(texEnvMapDiffuse, directionToSphericalEnvmap(n)).rgb;
		//IBL specular
		radiance += specularIBL(f0, roughness, n, v);
	}
	else
	{
		vec3 l = lightDir1;
		float irradiance = max(dot(l, n), 0.f) * lightIntensity1;
		if(irradiance > 0.f)
		{
			vec3 brdf = brdfMicrofacet(l, v, n, metallic, roughness, baseColor); 
			radiance += brdf * irradiance * lightColor1;
		}

		l = lightDir2;
		irradiance = max(dot(l, n), 0.f) * lightIntensity2;
		if(irradiance > 0.f)
		{
			vec3 brdf = brdfMicrofacet(l, v, n, metallic, roughness, baseColor); 
			radiance += brdf * irradiance * lightColor2;
		}
	}

	//occlusion 
	float occ = 1.f;
	if(enableAO)
	{
		if(seperateAO)
			occ = texture(texAO, fsin.texCoord).r;
		else
			occ =  texture(texMatArray, vec3(fsin.texCoord, 2.f)).r;
	}

	float alpha = 1.f;
	if(blended)
	{
		if(alphaDiffuse)
			alpha = texture(texMatArray, vec3(fsin.texCoord, 0.f)).a;
		else
			alpha = opacity;
	}
		
	fragColor = vec4(lin2rgb(radiance * occ), alpha);
}

layout(index = 2)
subroutine(RenderPass)
void renderBaseColor()
{
	fragColor = vec4(texture(texMatArray, vec3(fsin.texCoord, 0.f)).rgb, 1.f);
}

layout(index = 3)
subroutine(RenderPass)
void renderEmissive()
{
	vec3 emissiveCol = vec3(0.f);
	if(emissive)
		emissiveCol = texture(texMatArray, vec3(fsin.texCoord, 3.f)).rgb;

	fragColor = vec4(emissiveCol, 1.f);
}

layout(index = 4)
subroutine(RenderPass)
void renderAlpha()
{
	float alpha = 1.f;
	if(blended)
	{
		if(alphaDiffuse)
			alpha = texture(texMatArray, vec3(fsin.texCoord, 0.f)).a;
		else
			alpha = opacity;
	}

	fragColor = vec4(vec3(alpha), 1.f);
}

layout(index = 5)
subroutine(RenderPass)
void renderOcclusion()
{
	float AO = 1.f;
	if(seperateAO)
		AO = texture(texAO, fsin.texCoord).r;
	else
		AO =  texture(texMatArray, vec3(fsin.texCoord, 2.f)).r;

	fragColor = vec4(vec3(AO), 1.f);
}

layout(index = 6)
subroutine(RenderPass)
void renderRoughnessMetallic()
{
	fragColor = vec4(texture(texMatArray, vec3(fsin.texCoord, 2.f)).gb, 0.f, 1.f);
}

layout(index = 7)
subroutine(RenderPass)
void renderRoughness()
{
	fragColor = vec4(vec3(texture(texMatArray, vec3(fsin.texCoord, 2.f)).g), 1.f);
}

layout(index = 8)
subroutine(RenderPass)
void renderMetallic()
{
	fragColor = vec4(vec3(texture(texMatArray, vec3(fsin.texCoord, 2.f)).b), 1.f);
}

layout(index = 9)
subroutine(RenderPass)
void renderNormalMapped()
{
	fragColor = vec4(normalize(fsin.TBN * (texture(texMatArray, vec3(fsin.texCoord, 1.f)).rgb * 2.0  - 1.0)), 1.f);
}

layout(index = 10)
subroutine(RenderPass)
void renderNormalTexture()
{
	fragColor = vec4(texture(texMatArray, vec3(fsin.texCoord, 1.f)).rgb, 1.f);
}

layout(index = 11)
subroutine(RenderPass)
void renderTexCoord()
{
	fragColor = vec4(fsin.texCoord, 0.f, 1.f);
}

subroutine uniform RenderPass renderPass;

void main()
{
	renderPass();
}


