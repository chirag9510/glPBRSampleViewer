#pragma once
#include "Camera.h"
#include "GeometryLoader.h"
#include <memory>
#include <SDL.h>

class Renderer
{
public:
	Renderer();
	~Renderer();
	void Run();

private:
	bool InitApp();
	bool ReadInitJSON();
	void Destroy();

	void CreatePreFilterEnvMaps(const std::string strHDRi);
	void CreatePreFilterDiffuseMap(const GLuint& fboIBLPreFilter, const GLuint& texHDR, const GLuint& vaoQuad);
	void CreatePreFilterSpecularMap(const GLuint& fboIBLPreFilter, const GLuint& texHDR, const GLuint& vaoQuad);
	void CreateBRDFIntegrationMap(const GLuint& fboIBLPreFilter, const GLuint& vaoQuad);
	void CreateHDRCubemap(const GLuint& fbo, const GLuint& texHDR, const GLuint& vaoQuad);															//for skybox

	SDL_Window* mWindow;
	SDL_GLContext mContext;
	int iWndWidth, iWndHeight;
	bool bFullscreen, bMultiSample, bCullBackFace;
	int iMSLevel;
	std::string strShaderDir;
	
	std::unique_ptr<Camera> pCamera;
	std::unique_ptr<GeometryLoader> pGeoLoader;

	GLuint texIBLDiffuse, texIBLSpecular, texBRDFIntegration, texCMSkybox, iMaxMipMaps;
	
	//for imgui
	std::vector<std::string> vecHDRIs;
	std::vector<std::string> vecModels;
	std::map<std::string, CameraDefaultsPerModel> mapCamDefPerModel;					//key is the model name
	std::string strComboHDRI;															//list of HDR backgrounds
};

