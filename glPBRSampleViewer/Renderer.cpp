#include "Renderer.h"
#include "Shader.h"
#include <json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"

Renderer::Renderer() :
	pGeoLoader(std::make_unique<GeometryLoader>()),
	pCamera(std::make_unique<Camera>()),
	iWndWidth(1280),
	iWndHeight(720),
	iMaxMipMaps(6),
	bCullBackFace(true),
	bFullscreen(false),
	bMultiSample(true),
	iMSLevel(8)
{
}

void Renderer::Run()
{
	if (!InitApp())
		return;

	CreatePreFilterEnvMaps(vecHDRIs[0]);

	Model model, modelSkybox;
	if (!pGeoLoader->LoadModel(model, vecModels[0]))
		return;
	if (!pGeoLoader->LoadModel(modelSkybox, "skybox"))								
		return;

	pCamera->Init();
	pCamera->fRadius = mapCamDefPerModel[vecModels[0]].fRadius;
	pCamera->SetTarget(mapCamDefPerModel[vecModels[0]].vTarget);
	pCamera->fRotateSensitivity = mapCamDefPerModel[vecModels[0]].fRotationSensitivity;
	pCamera->fZoomSensitivity = mapCamDefPerModel[vecModels[0]].fZoomSensitivity;
	pCamera->UpdateMatView();

	bool bEnableIBL = true, bEnableAO = true, bDrawSkybox = true;
	float fLightIntensity1 = 1.f, fLightIntensity2 = 1.f, fReflectance = 0.5f;
	glm::vec3 vLightDir1 = glm::vec3(0.f, 1.f, -1.f);
	glm::vec3 vLightDir2 = glm::vec3(0.f, 1.f, 1.f);
	glm::vec3 vLightColor1 = glm::vec3(1.f, 1.f, 1.f);
	glm::vec3 vLightColor2 = glm::vec3(1.f, 1.f, 1.f);

	Shader shader(strShaderDir + "render.vert", strShaderDir + "render.frag");
	glUseProgram(shader.program);
	glUniform1f(0, fReflectance);
	glUniform1f(1, (float)iMaxMipMaps);
	glUniform1i(7, bEnableIBL);
	glUniform1i(8, bEnableAO);
	glUniform3fv(9, 1, glm::value_ptr(vLightDir1));
	glUniform1f(10, fLightIntensity1);
	glUniform3fv(11, 1, glm::value_ptr(vLightDir2));
	glUniform1f(12, fLightIntensity2);
	glUniform3fv(13, 1, glm::value_ptr(vLightColor1));
	glUniform3fv(14, 1, glm::value_ptr(vLightColor2));
	glBindTextureUnit(1, texIBLDiffuse);
	glBindTextureUnit(2, texIBLSpecular);
	glBindTextureUnit(3, texBRDFIntegration);
	glBindTextureUnit(5, texCMSkybox);

	//imgui init
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;				
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(mWindow, mContext);			
	ImGui_ImplOpenGL3_Init();

	ImVec2 imWndDimens(iWndWidth / 4.f, iWndHeight - 10);
	float fColor[3] = { 0.05, 0.05, 0.15f };
	int inComboDebug = 0, inComboHDRi = 0, inComboModels = 0;										//index for imgui combo
	const char* szComboDebugChannel = "None\0Base Color\0Emissive\0Alpha\0Occlusion\0Roughness Metallic\0Roughness\0Metallic\0Normal Mapped\0Normal Texture\0Tex Coordinates0";
	
	float fDeltaTime = 0.f;
	Uint32 iTicksLastFrame = SDL_GetTicks(), iTicksCurrent = 0;
	GLuint subVertRender = 0, subFragRender = 1;							//subroutines , default model PBR rendering at 1

	SDL_Event e;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glClearColor(fColor[0], fColor[1], fColor[2], 1.f);
	while (true)
	{
		iTicksCurrent = SDL_GetTicks();
		fDeltaTime = (float)(iTicksCurrent - iTicksLastFrame) / 1000.f;
		iTicksLastFrame = iTicksCurrent;

		while (SDL_PollEvent(&e))
		{
			ImGui_ImplSDL2_ProcessEvent(&e);
			if(!ImGui::GetIO().WantCaptureMouse)
				pCamera->Update(e, fDeltaTime);

			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
				return;
		}

		//Imgui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(5.f, 5.f));
		ImGui::SetNextWindowSize(imWndDimens);
		{
			ImGui::Begin("Display", 0);                         
			
			imWndDimens.x = glm::clamp(ImGui::GetWindowWidth(), iWndWidth / 4.f, iWndWidth - 10.f);				//from SetNextWindowPos() 2 * x value

			ImGui::SeparatorText("Controls");
			ImGui::TextWrapped("Left Mouse Drag: Rotate Camera");
			ImGui::TextWrapped("Right Mouse Drag: Move Camera");
			ImGui::TextWrapped("Mouse Wheel: Zoom Camera");
			ImGui::TextWrapped("Control + Click: Set custom values for Slider widgets");
			ImGui::TextWrapped("Escape key: Quit");
			
			ImGui::Spacing();
			ImGui::SeparatorText("Select Model");
			if (ImGui::BeginCombo("Model", vecModels[inComboModels].c_str()))
			{
				for (int n = 0; n < vecModels.size(); n++)
				{
					const bool bSelected = (inComboModels == n);
					if (ImGui::Selectable(vecModels[n].c_str(), bSelected))
					{
						inComboModels = n;

						//destroy current, load new
						model.Destroy();
						std::string strModel = vecModels[inComboModels];
						pGeoLoader->LoadModel(model, strModel);
						pCamera->fRadius = mapCamDefPerModel[strModel].fRadius;
						pCamera->SetTarget(mapCamDefPerModel[strModel].vTarget);
						pCamera->fYaw = PI_2THIRDS;
						pCamera->fPitch = 0.f;
						pCamera->fRotateSensitivity = mapCamDefPerModel[strModel].fRotationSensitivity;
						pCamera->fZoomSensitivity = mapCamDefPerModel[strModel].fZoomSensitivity;
						pCamera->UpdateMatView();
					}

					//set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::Spacing();
			ImGui::SeparatorText("IBL Background");
			if (ImGui::BeginCombo("Background", vecHDRIs[inComboHDRi].c_str()))
			{
				for (int n = 0; n < vecHDRIs.size(); n++)
				{
					const bool bSelected = (inComboHDRi == n);
					if (ImGui::Selectable(vecHDRIs[n].c_str(), bSelected))
					{
						inComboHDRi = n;
						//reset the prefilter diffuse/specular, cubemap textures
						glDeleteTextures(1, &texIBLSpecular);
						glDeleteTextures(1, &texIBLDiffuse);
						glDeleteTextures(1, &texCMSkybox);
						glDeleteTextures(1, &texBRDFIntegration);

						CreatePreFilterEnvMaps(vecHDRIs[inComboHDRi]);

						glUseProgram(shader.program);
						glBindTextureUnit(1, texIBLDiffuse);
						glBindTextureUnit(2, texIBLSpecular);
						glBindTextureUnit(3, texBRDFIntegration);
						glBindTextureUnit(5, texCMSkybox);
					}
					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::Spacing();
			ImGui::SeparatorText("Reflectance");
			if (ImGui::SliderFloat("Reflect", &fReflectance, 0.f, 1.f))
				glUniform1f(0, fReflectance);

			ImGui::Spacing();
			ImGui::SeparatorText("Debug Channel");
			if (ImGui::Combo("Channel", &inComboDebug, szComboDebugChannel))
			{
				subFragRender = inComboDebug + 1;
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Separator();
			if (ImGui::Checkbox("IBL", &bEnableIBL))
				glUniform1i(7, bEnableIBL);

			if (!bEnableIBL)
			{
				ImGui::Spacing();
				if (ImGui::SliderFloat3("Light Dir 1", (float*)&vLightDir1, -1.f, 1.f))
					glUniform3fv(9, 1, glm::value_ptr(vLightDir1));
				ImGui::Spacing();
				if (ImGui::ColorEdit3("Light Color 1", (float*)&vLightColor1))
					glUniform3fv(13, 1, glm::value_ptr(vLightColor1));
				ImGui::Spacing();
				if (ImGui::SliderFloat("Light Intensity 1", &fLightIntensity1, 0.f, 2.f))
					glUniform1f(10, fLightIntensity1);
				ImGui::Spacing();
				if (ImGui::SliderFloat3("Light Dir 2", (float*)&vLightDir2, -1.f, 1.f))
					glUniform3fv(11, 1, glm::value_ptr(vLightDir2));
				ImGui::Spacing();
				if (ImGui::ColorEdit3("Light Color 2", (float*)&vLightColor2))
					glUniform3fv(14, 1, glm::value_ptr(vLightColor2));
				ImGui::Spacing();
				if (ImGui::SliderFloat("Light Intensity 2", &fLightIntensity2, 0.f, 2.f))
					glUniform1f(12, fLightIntensity2);
				ImGui::Spacing();
			}

			ImGui::Separator();
			ImGui::Checkbox("Draw Skybox", &bDrawSkybox);
			if (!bDrawSkybox)
			{
				if (ImGui::ColorEdit3("Background Color", fColor))
					glClearColor(fColor[0], fColor[1], fColor[2], 1.f);
				ImGui::Spacing();
			}

			ImGui::Separator();
			if (ImGui::Checkbox("Occlusion", &bEnableAO))
				glUniform1i(8, bEnableAO);

			ImGui::Separator();
			ImGui::Checkbox("Cull Back face", &bCullBackFace);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::SeparatorText("Camera");
			
			ImGui::Spacing();
			if (ImGui::SliderFloat("FOV", &pCamera->fFOV, 10.f, 150.f))
				pCamera->UpdateMatProjection();

			ImGui::Separator();
			ImGui::SliderFloat("Rotation Sensitivity", &pCamera->fRotateSensitivity, 0.1f, 3.f);

			ImGui::Separator();
			ImGui::SliderFloat("Zoom Sensitivity", &pCamera->fZoomSensitivity, 0.1f, 30.f);

			ImGui::Separator();
			if (ImGui::SliderFloat("Far", &pCamera->fFar, 0.1f, 1000.f))
				pCamera->UpdateMatProjection();
			
			ImGui::Separator();
			if (ImGui::SliderFloat("Near", &pCamera->fNear, 0.01f, 2.f))
				pCamera->UpdateMatProjection();

			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.f, 1.f, 0.4f, 1.f), "//chirag");
			ImGui::TextColored(ImVec4(0.f, 1.f, 0.4f, 1.f), "chirag.101095@gmail.com");

			ImGui::End();
		}

		//Rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (bDrawSkybox)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glDepthFunc(GL_LEQUAL);																		// the z value will be 1.f in vertex shader which is why depth func needs this					

			subVertRender = 0;
			glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &subVertRender);
			glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subVertRender);
			glBindVertexArray(modelSkybox.vao);
			glDrawElements(GL_TRIANGLES, modelSkybox.meshes[0].count, GL_UNSIGNED_INT, 0);
			glDepthFunc(GL_LESS);
		}
		
		//Default Draw IBL model
		glCullFace(GL_BACK);
		if(!bCullBackFace)
			glDisable(GL_CULL_FACE);

		subVertRender = 1;
		glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &subVertRender);
		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subFragRender);

		GLuint iOpaqueMat = 0;
		glBindVertexArray(model.vao);
		for (auto& mesh : model.meshes)
		{
			if (iOpaqueMat == model.iNumOpaqueMeshes)
			{
				glEnable(GL_BLEND);
				glEnable(GL_CULL_FACE);									
			}
			else
				iOpaqueMat++;

			glUniform1i(2, mesh.material.bEmissive);
			glUniform1i(3, mesh.material.bSeperateAO);
			glUniform1i(4, mesh.material.bBlended);
			glUniform1i(5, mesh.material.bAlphaDiffuse);
			glUniform1f(6, mesh.material.fOpacity);
			glBindTextureUnit(0, mesh.material.texArray);
			glBindTextureUnit(4, mesh.material.texAO);
			glDrawElementsBaseVertex(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, mesh.offset, mesh.baseVertex);
		}
		
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);

		//imgui
		ImGui::Render();
//		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());	

		SDL_GL_SwapWindow(mWindow);
	}
}

void Renderer::CreatePreFilterEnvMaps(const std::string strHDRi)
{
	GLuint fboIBL;
	glCreateFramebuffers(1, &fboIBL);
	glBindFramebuffer(GL_FRAMEBUFFER, fboIBL);
	GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_NONE};
	glDrawBuffers(2, buffers);

	Quad quad;
	GLuint texHDR;
	pGeoLoader->LoadQuad(quad);
	pGeoLoader->LoadHDRI(texHDR, std::string(strHDRi + ".hdr").c_str(), iMaxMipMaps);

	Shader shader(strShaderDir + "prefilter.vert", strShaderDir + "prefilter.frag");
	glUseProgram(shader.program);
	glUniform1i(0, iWndWidth);
	glUniform1i(1, iWndHeight);
	glBindTextureUnit(0, texHDR);

	CreatePreFilterDiffuseMap(fboIBL, texHDR, quad.vao);
	CreatePreFilterSpecularMap(fboIBL, texHDR, quad.vao);
	CreateBRDFIntegrationMap(fboIBL, quad.vao);

	// for skybox
	CreateHDRCubemap(fboIBL, texHDR, quad.vao);
	
	//back to default;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, iWndWidth, iWndHeight);

	glDeleteTextures(1, &texHDR);
	glDeleteFramebuffers(1, &fboIBL);
}

void Renderer::CreatePreFilterDiffuseMap(const GLuint& fboIBLPreFilter, const GLuint& texHDR, const GLuint& vaoQuad)
{
	GLuint sub = 0;
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &sub);

	//mantain 1:2 ratio of the hdr texture, using smaller texture for performance
	int samples = 4096;
	float mimmap = 5.f;
	int texWidth = 256, texHeight = 128;		
	glUniform1i(2, samples);
	glUniform1f(3, mimmap);

	glCreateTextures(GL_TEXTURE_2D, 1, &texIBLDiffuse);
	glTextureStorage2D(texIBLDiffuse, 1, GL_RGBA16F, texWidth, texHeight);
	glTextureParameteri(texIBLDiffuse, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texIBLDiffuse, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glNamedFramebufferTexture(fboIBLPreFilter, GL_COLOR_ATTACHMENT0, texIBLDiffuse, 0);

	//render once 
	glBindFramebuffer(GL_FRAMEBUFFER, fboIBLPreFilter);
	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vaoQuad);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::CreatePreFilterSpecularMap(const GLuint& fboIBLPreFilter, const GLuint& texHDR, const GLuint& vaoQuad)
{
	GLuint sub = 1;
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &sub);

	int texWidth = 1024, texHeight = 512;					//mantain 1:2 ratio of the hdr texture

	glCreateTextures(GL_TEXTURE_2D, 1, &texIBLSpecular);
	glTextureStorage2D(texIBLSpecular, iMaxMipMaps, GL_RGBA16F, texWidth, texHeight);
	glTextureParameteri(texIBLSpecular, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);					//dont sample out of bounds of texture
	glTextureParameteri(texIBLSpecular, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texIBLSpecular, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(texIBLSpecular, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateTextureMipmap(texIBLSpecular);
	
	//first 2 mipmaps with 1024 samples
	glUniform1i(2, 1024);
	glBindFramebuffer(GL_FRAMEBUFFER, fboIBLPreFilter);
	for (int i = 0; i < iMaxMipMaps; i++)
	{
		glNamedFramebufferTexture(fboIBLPreFilter, GL_COLOR_ATTACHMENT0, texIBLSpecular, i);
		glViewport(0, 0, texWidth * pow(0.5f, i), texHeight * pow(0.5f, i));

		if (i > 1)
			glUniform1i(2, 4096);
		glUniform1f(3, (float)i);
		glUniform1f(4, 0.2f * (float)i);
		glBindVertexArray(vaoQuad);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	}
}

void Renderer::CreateBRDFIntegrationMap(const GLuint& fboIBLPreFilter, const GLuint& vaoQuad)
{
	GLuint sub = 2;
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &sub);
	GLenum buffers[] = { GL_NONE, GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(2, buffers);

	int texWidth = 256, texHeight = 256;					//mantain 1:2 ratio of the hdr texture
	glUniform1i(2, 4096);

	glCreateTextures(GL_TEXTURE_2D, 1, &texBRDFIntegration);
	glTextureStorage2D(texBRDFIntegration, 1, GL_RG16F, texWidth, texHeight);
	glTextureParameteri(texBRDFIntegration, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);				//dont sample out of bounds of texture
	glTextureParameteri(texBRDFIntegration, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texBRDFIntegration, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texBRDFIntegration, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glNamedFramebufferTexture(fboIBLPreFilter, GL_COLOR_ATTACHMENT0, texBRDFIntegration, 0);

	//render once 
	glBindFramebuffer(GL_FRAMEBUFFER, fboIBLPreFilter);
	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(vaoQuad);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::CreateHDRCubemap(const GLuint& fbo, const GLuint& texHDR, const GLuint& vaoQuad)
{
	Shader shader(strShaderDir + "panoramaToCubemap.vert", strShaderDir + "panoramaToCubemap.frag");
	glUseProgram(shader.program);
	glBindTextureUnit(0, texIBLSpecular);
	
	GLuint texDim = 1024;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texCMSkybox);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texCMSkybox);
	for (unsigned int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, texDim, texDim, 0, GL_RGB, GL_FLOAT, nullptr);
	
	glTextureParameteri(texCMSkybox, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texCMSkybox, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texCMSkybox, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texCMSkybox, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texCMSkybox, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glViewport(0, 0, texDim, texDim);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBindVertexArray(vaoQuad);									//dummy
	for (unsigned int i = 0; i < 6; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, texCMSkybox, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1i(0, i);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

bool Renderer::ReadInitJSON()
{
	// Read app settings data from data.json
	std::string strFilePos = "./data.json";
	std::ifstream jsonFile(strFilePos);
	if (jsonFile.is_open())
	{
		nlohmann::json data = nlohmann::json::parse(jsonFile);
		iWndWidth = data["WndWidth"];
		iWndHeight = data["WndHeight"];
		bMultiSample = data["MultiSample"];
		iMSLevel = data["Samples"];
		bFullscreen = data["Fullscreen"];
		
		std::string strLocalDir = data["LocalDir"];
		strShaderDir = data["ShaderDir"];
		strShaderDir = strLocalDir + strShaderDir;
		
		const auto dataHDRi = data["HDRIs"];
		for (const auto& strHDRI : dataHDRi)
		{
			vecHDRIs.emplace_back(strHDRI);
		}

		const auto dataModels = data["Models"];
		for (const auto& model : dataModels)
		{
			vecModels.emplace_back(model["name"]);
			mapCamDefPerModel[model["name"]] = CameraDefaultsPerModel(
				model["camRadius"], 
				glm::vec3(model["camTarget"][0], model["camTarget"][1], model["camTarget"][2]),
				model["camRotSens"],
				model["camZoomSens"]);
		}

		pGeoLoader->LoadData(data);
		pCamera->LoadData(data);

		jsonFile.close();
	}
	else
	{
		spdlog::error("Failed to read data.json at location : " + strFilePos);
		return false;
	}

	return true;
}

bool Renderer::InitApp()
{
	if (!ReadInitJSON())
		return false;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	if (bMultiSample)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, iMSLevel);
	}
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
	if (bFullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;

	mWindow = SDL_CreateWindow("glPBRSampleViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, iWndWidth, iWndHeight, flags);
	mContext = SDL_GL_CreateContext(mWindow);
	SDL_GL_MakeCurrent(mWindow, mContext);
	SDL_GL_SetSwapInterval(1);											//vsync
	
	glewExperimental = true;
	glewInit();

	return true;
}

void Renderer::Destroy()
{
	glDeleteTextures(1, &texIBLSpecular);
	glDeleteTextures(1, &texIBLDiffuse);
	glDeleteTextures(1, &texCMSkybox);
	glDeleteTextures(1, &texBRDFIntegration);

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	SDL_GL_DeleteContext(mContext);
	SDL_DestroyWindow(mWindow);
	SDL_Quit();

	spdlog::info("Renderer Destroyed");
}

Renderer::~Renderer()
{
	Destroy();
}








