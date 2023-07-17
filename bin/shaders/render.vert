#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;

layout(std140, binding = 0) uniform PersMat
{
	mat4 proj;
	mat4 view;
	vec3 cameraPos;
}persMat;

out VSOut
{
    vec3 viewDir;
	vec2 texCoord;
	mat3 TBN;
	
	//skybox only
	vec3 texCoordCube;
}vsout;

subroutine void RenderPassVert();

layout(index = 0)
subroutine(RenderPassVert)
void renderSkybox()
{
	vec4 pos = persMat.proj * mat4(mat3(persMat.view)) * vec4(aPos, 1.f);
	gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);				
	vsout.texCoordCube = vec3(aPos.x, aPos.y, -aPos.z);				
}

layout(index = 1)
subroutine(RenderPassVert)
void renderDefault()
{
	gl_Position = persMat.proj * persMat.view * vec4(aPos, 1.f);
	
	vsout.viewDir = normalize(persMat.cameraPos - aPos);
	vsout.TBN = mat3(aTangent, cross(aNormal, aTangent), aNormal);
	vsout.texCoord = aTexCoord;
}

subroutine uniform RenderPassVert renderPassVert;

void main()
{
	renderPassVert();
}