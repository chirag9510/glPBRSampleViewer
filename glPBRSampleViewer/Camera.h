#pragma once
#include <SDL_events.h>
#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <json.hpp>

constexpr float PI = 3.141592653589793;
constexpr float PI_2 = PI * 2.f;
constexpr float PI_HALF = PI / 2.f - 0.001f;								//for clamping pitch
constexpr float PI_2THIRDS = PI_2 / 3.f;						

//default camera values for each loaded model, used by renderer
struct CameraDefaultsPerModel
{
	float fRadius;
	glm::vec3 vTarget;
	float fRotationSensitivity;
	float fZoomSensitivity;
	CameraDefaultsPerModel(float fRadius = 2.f, glm::vec3 vTarget = glm::vec3(0.f, 0.f, 0.f), float fRotationSensitivity = 0.3f, float fZoomSensitivity = 3.f)
		: fRadius(fRadius), vTarget(vTarget), fRotationSensitivity(fRotationSensitivity), fZoomSensitivity(fZoomSensitivity) {}
};

struct PersMatrices
{
	glm::mat4 matProj;
	glm::mat4 matView;
	glm::vec3 vEyePos;
};

class Camera
{
public:
	Camera();
	void Init();
	void Update(const SDL_Event& e, const float& fDeltaTime);
	void UpdateMatProjection();
	void UpdateMatView();
	void LoadData(const nlohmann::json& data);
	void UpdateUBO();

	glm::mat4 GetProjMatrix() const;
	glm::mat4 GetViewMatrix() const;
	void SetTarget(const glm::vec3 vTarget);

	float fYaw, fPitch, fRadius;
	float fRotateSensitivity, fZoomSensitivity;
	float fFOV, fAspectRatio, fFar, fNear;

		
private:

	void RotateYaw(const float fRotateRad);
	void RotatePitch(const float fRotateRad);
	void Zoom(const float fDistance);
	void MoveHorizontal(const float fDistance);
	void MoveVertical(const float fDistance);

	PersMatrices persMatrices;
	GLuint uboPerspec;
	glm::vec3 vTarget, vUp;
	bool bLeftMBDown, bRightMBDown;
};