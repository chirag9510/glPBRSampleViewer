#pragma once
#include <string>
#include <vector>
#include <string>
#include <json.hpp>
#include <GL/glew.h>
#include <assimp/scene.h>

struct Material
{
	GLuint texArray;
	GLuint texAO;									//if AO texture is seperate from RoughnessMetallic tex
	bool bSeperateAO;								//AO is in seperate texture instead of bieng in arm(ao, roughness, metallic)
	bool bEmissive;
	//transparency
	bool bBlended;								
	bool bAlphaDiffuse;								//if true, alpha value is in diffuse png .a, else its in AI_MATKEY_OPACITY assimp key and stores in fOpacity
	float fOpacity;									//default alpha value for blended translucency

	Material(): bSeperateAO(false), bEmissive(false), bBlended(false), texAO(0), texArray(0), fOpacity(0.15f), bAlphaDiffuse(true) {}
};

struct Mesh
{
	Material material;
	GLuint baseVertex;
	GLuint count;
	void* offset;
};

class Model
{
public:
	GLuint vao, vbo, ebo;
	GLuint iNumOpaqueMeshes;														
	std::vector<Mesh> meshes;
	Model() : vao(0), iNumOpaqueMeshes(0) {}
	~Model() 
	{
		if(vao)
			Destroy();
	}
	 
	void Destroy()
	{
		iNumOpaqueMeshes = 0;
		for (auto& mesh : meshes)
		{
			glDeleteTextures(1, &mesh.material.texArray);
			if (mesh.material.texAO)
				glDeleteTextures(1, &mesh.material.texAO);
		}
		meshes.clear();

		glBindVertexArray(vao);
		glDeleteBuffers(1, &ebo);
		glDeleteBuffers(1, &vbo);
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
};

struct Quad
{
	GLuint vao, vbo, ebo;
	Quad() : vao(0) {}
	~Quad() 
	{
		glBindVertexArray(vao);
		glDeleteBuffers(1, &ebo);
		glDeleteBuffers(1, &vbo);
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);
	}
};

class GeometryLoader
{
public:
	bool LoadModel(Model& model, const std::string strModel);
	void LoadQuad(Quad& quad);
	void LoadHDRI(GLuint& texture, const char* szTexFilename, const int& mipmapLevels);

	void LoadData(const nlohmann::json& data);

private:	
	void LoadMaterials(const aiScene* scene, std::map<GLuint, Material>& mapMaterials);
	void LoadTextureIntoArray(GLuint& texarray, const std::string& strTexPath, const GLuint texIndex, bool bInitTexArray = false, const GLuint iTotalTextures = 3);
	void LoadTexture2D(GLuint& tex, const std::string& strTexPath);

	std::string strHDRIDir, strModelDir;
};
