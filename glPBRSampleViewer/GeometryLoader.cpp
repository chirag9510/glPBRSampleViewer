#include "GeometryLoader.h"
#include <spdlog/spdlog.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/vec4.hpp>

#include <assimp/GltfMaterial.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool GeometryLoader::LoadModel(Model& model, const std::string strModel)
{
	std::string strModelPath = strModelDir + strModel + ".gltf";
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(strModelPath, aiProcessPreset_TargetRealtime_Quality | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
	if (scene == nullptr)
	{
		spdlog::error("Failed to load aiScene for model at : " + strModelPath);
		return false;
	}
	
	std::map<GLuint, Material> mapMaterials;
	LoadMaterials(scene, mapMaterials);

	glCreateVertexArrays(1, &model.vao);
	glCreateBuffers(1, &model.vbo);
	glCreateBuffers(1, &model.ebo);

	//reserve memory for all submeshes, v/n/t/tangent, initialized to 0.0f
	GLuint iSizeVertex = sizeof(float) * 11;
	GLuint totalVertices = 0, totalElements = 0;
	for (int i = 0; i < scene->mNumMeshes; i++)
	{
		totalVertices += scene->mMeshes[i]->mNumVertices;
		totalElements += scene->mMeshes[i]->mNumFaces * 3;
	}

	glNamedBufferStorage(model.vbo, iSizeVertex * totalVertices, NULL, GL_MAP_WRITE_BIT);
	glNamedBufferStorage(model.ebo, sizeof(unsigned int) * totalElements, NULL, GL_MAP_WRITE_BIT);

	//map into buffers
	GLuint baseVertex = 0, firstIndex = 0;
	for (int i = 0; i < scene->mNumMeshes; i++)
	{
		Mesh mesh;
		aiMesh* aimesh = scene->mMeshes[i];

		GLuint index = 0;
		float* vertex = (float*)glMapNamedBufferRange(model.vbo, iSizeVertex * baseVertex, iSizeVertex * aimesh->mNumVertices, GL_MAP_WRITE_BIT);
		for (int v = 0; v < aimesh->mNumVertices; v++)
		{
			vertex[index++] = aimesh->mVertices[v].x;
			vertex[index++] = aimesh->mVertices[v].y;
			vertex[index++] = aimesh->mVertices[v].z;
			vertex[index++] = aimesh->mNormals[v].x;
			vertex[index++] = aimesh->mNormals[v].y;
			vertex[index++] = aimesh->mNormals[v].z;

			if (aimesh->HasTextureCoords(0))
			{
				vertex[index++] = aimesh->mTextureCoords[0][v].x;
				vertex[index++] = aimesh->mTextureCoords[0][v].y;
			}
			else
			{
				vertex[index++] = 0.f;
				vertex[index++] = 0.f;
			}
			if (aimesh->HasTangentsAndBitangents())
			{
				vertex[index++] = aimesh->mTangents[v].x;
				vertex[index++] = aimesh->mTangents[v].y;
				vertex[index++] = aimesh->mTangents[v].z;
			}
			else
			{
				vertex[index++] = 0.f;
				vertex[index++] = 0.f;
				vertex[index++] = 0.f;
			}
		}
		glUnmapNamedBuffer(model.vbo);

		index = 0;
		unsigned int* element = (unsigned int*)glMapNamedBufferRange(model.ebo, sizeof(unsigned int) * firstIndex, sizeof(unsigned int) * aimesh->mNumFaces * 3, GL_MAP_WRITE_BIT);
		for (int f = 0; f < aimesh->mNumFaces; f++)
		{
			element[index++] = aimesh->mFaces[f].mIndices[0];
			element[index++] = aimesh->mFaces[f].mIndices[1];
			element[index++] = aimesh->mFaces[f].mIndices[2];
		}
		glUnmapNamedBuffer(model.ebo);

		mesh.baseVertex = baseVertex;
		mesh.offset = (void*)(sizeof(unsigned int) * firstIndex);
		mesh.count = aimesh->mNumFaces * 3;
		mesh.material = mapMaterials[aimesh->mMaterialIndex];

		//sort, blended/transparent material meshes should be drawn last, only after all opaque materials are drawn
		if (!mesh.material.bBlended)
			model.meshes.insert(model.meshes.begin() + model.iNumOpaqueMeshes++, mesh);
		else
			model.meshes.emplace_back(mesh);

		firstIndex += aimesh->mNumFaces * 3;
		baseVertex += aimesh->mNumVertices;

	}

	glVertexArrayAttribBinding(model.vao, 0, 0);
	glVertexArrayAttribFormat(model.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(model.vao, 0);
	glVertexArrayAttribBinding(model.vao, 1, 0);
	glVertexArrayAttribFormat(model.vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
	glEnableVertexArrayAttrib(model.vao, 1);
	glVertexArrayAttribBinding(model.vao, 2, 0);
	glVertexArrayAttribFormat(model.vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
	glEnableVertexArrayAttrib(model.vao, 2);
	glVertexArrayAttribBinding(model.vao, 3, 0);
	glVertexArrayAttribFormat(model.vao, 3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8);
	glEnableVertexArrayAttrib(model.vao, 3);

	glVertexArrayVertexBuffer(model.vao, 0, model.vbo, 0, iSizeVertex);
	glVertexArrayElementBuffer(model.vao, model.ebo);

	return true;
}

void GeometryLoader::LoadMaterials(const aiScene* scene, std::map<GLuint, Material>& mapMaterials)
{
	aiString aistrDiffuse, aistrNormals, aistrARM, aistrEmissive, aistrAO, aistrBlend;
	bool bInitTexture = true;
	for (GLuint i = 0; i < scene->mNumMaterials; i++)
	{
		Material& meshMaterial = mapMaterials[i];
		aiMaterial* material = scene->mMaterials[i];
		material->GetTexture(aiTextureType_DIFFUSE, 0, &aistrDiffuse);
		material->GetTexture(aiTextureType_NORMALS, 0, &aistrNormals);
		material->GetTexture(aiTextureType_UNKNOWN, 0, &aistrARM);
		if (material->GetTexture(aiTextureType_EMISSIVE, 0, &aistrEmissive) == aiReturn_SUCCESS)		//only load if any
		{
			LoadTextureIntoArray(meshMaterial.texArray, std::string(strModelDir + aistrEmissive.C_Str()), 3, true, 4);
			meshMaterial.bEmissive = true;
			bInitTexture = false;
		}

		LoadTextureIntoArray(meshMaterial.texArray, std::string(strModelDir + aistrDiffuse.C_Str()), 0, bInitTexture);
		LoadTextureIntoArray(meshMaterial.texArray, std::string(strModelDir + aistrNormals.C_Str()), 1);
		LoadTextureIntoArray(meshMaterial.texArray, std::string(strModelDir + aistrARM.C_Str()), 2);

		//occlusion, if seperate from the roughnessmetallic map(ARM)
		//also dont load ARM map twice
		if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &aistrAO) == aiReturn_SUCCESS && aistrAO != aistrARM)
		{
			LoadTexture2D(meshMaterial.texAO, std::string(strModelDir + aistrAO.C_Str()));
			meshMaterial.bSeperateAO = true;
		}

		//transparent material
		if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, aistrBlend) == aiReturn_SUCCESS && std::string(aistrBlend.C_Str()) == "BLEND")			//if material is opaque or blend
		{
			meshMaterial.bBlended = true;
			float fOpacity = 0.f;
			if (material->Get(AI_MATKEY_OPACITY, fOpacity) == aiReturn_SUCCESS && fOpacity != 1.f)
			{
				meshMaterial.bAlphaDiffuse = false;
				meshMaterial.fOpacity = fOpacity;
			}
		}
	}
}

void GeometryLoader::LoadTextureIntoArray(GLuint& texarray, const std::string& strTexPath, const GLuint texIndex, bool bInitTexArray, const GLuint iTotalTextures)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(strTexPath.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
	if (data != nullptr)
	{
		if (bInitTexArray)
		{
			//init first time 
			glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &texarray);
			glTextureStorage3D(texarray, 1, GL_RGBA8, width, height, iTotalTextures);										//3 for diffuse/normal/arm/emissive
			glTextureParameteri(texarray, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(texarray, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTextureParameteri(texarray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(texarray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glGenerateTextureMipmap(texarray);
		}
		
		glTextureSubImage3D(texarray, 0, 0, 0, texIndex, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}
	else
		spdlog::error("Failed to load Texture at : " + strTexPath);

}

void GeometryLoader::LoadTexture2D(GLuint& texture, const std::string& strTexPath)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(strTexPath.c_str(), &width, &height, &nrChannels, STBI_rgb);
	if (data)
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureStorage2D(texture, 1, GL_RGB8, width, height);										
		glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);
	}
	else
		spdlog::error("Failed to load Texture at : " + strTexPath);
}

void GeometryLoader::LoadHDRI(GLuint& texture, const char* szTexFilename, const int& mipmapLevels)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	float* data = stbi_loadf(std::string(strHDRIDir + szTexFilename).c_str(), &width, &height, &nrChannels, 0);
	if (data != nullptr)
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureStorage2D(texture, mipmapLevels, GL_RGB16F, width, height);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, data);					
		glGenerateTextureMipmap(texture);

		stbi_image_free(data);
	}
	else
		spdlog::error("Failed to laod HDR texture at path : " + strHDRIDir + szTexFilename);
}

void GeometryLoader::LoadQuad(Quad& quad)
{
	GLfloat vertices[] =
	{
		-1.f, -1.f, 0.f, 0.f, 0.f,
		1.f, -1.f, 0.f, 1.f, 0.f,
		1.f, 1.f, 0.f, 1.f, 1.f,
		-1.f, 1.f, 0.f, 0.f, 1.f
	};
	unsigned int indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	glCreateVertexArrays(1, &quad.vao);
	glCreateBuffers(1, &quad.vbo);
	glCreateBuffers(1, &quad.ebo);

	glNamedBufferStorage(quad.vbo, sizeof(float) * 20, vertices, 0);
	glVertexArrayAttribBinding(quad.vao, 0, 0);
	glVertexArrayAttribFormat(quad.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(quad.vao, 0);
	glVertexArrayAttribBinding(quad.vao, 2, 0);
	glVertexArrayAttribFormat(quad.vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
	glEnableVertexArrayAttrib(quad.vao, 2);
	glVertexArrayVertexBuffer(quad.vao, 0, quad.vbo, 0, sizeof(float) * 5);

	glNamedBufferStorage(quad.ebo, sizeof(unsigned int) * 6, indices, 0);
	glVertexArrayElementBuffer(quad.vao, quad.ebo);
}

void GeometryLoader::LoadData(const nlohmann::json& data)
{
	std::string strLocalDir = data["LocalDir"];
	strHDRIDir = data["HDRIDir"];
	strHDRIDir = strLocalDir + strHDRIDir;
	strModelDir = data["ModelDir"];
	strModelDir = strLocalDir + strModelDir;

	spdlog::info(strLocalDir);
	spdlog::info(strHDRIDir);
	spdlog::info(strModelDir);
}