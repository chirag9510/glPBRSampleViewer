#include "Camera.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <GL/glew.h>
#include <spdlog/spdlog.h>


Camera::Camera() :
	bLeftMBDown(false),
	bRightMBDown(false),
	fYaw(PI_2THIRDS),
	fPitch(0.f),
	fRadius(2.f),
	fRotateSensitivity(.3f),
	fZoomSensitivity(3.f),
	vTarget(glm::vec3(0.f)),
	vUp(glm::vec3(0.f, 1.f, 0.f))
{
}

void Camera::LoadData(const nlohmann::json& data)
{
	fFOV = data["FOV"];
	fFar = data["Far"];
	fNear = data["Near"];
	int iWndWidth = data["WndWidth"];
	int iWndHeight = data["WndHeight"];
	fAspectRatio = (float)iWndWidth / (float)iWndHeight;
}

void Camera::Init()
{
	glCreateBuffers(1, &uboPerspec);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboPerspec);
	glNamedBufferStorage(uboPerspec, sizeof(PersMatrices), NULL, GL_MAP_WRITE_BIT);

	UpdateMatView();
	UpdateMatProjection();
}

void Camera::Update(const SDL_Event& e, const float& fDeltaTime)
{
	switch (e.type)
	{
	case SDL_MOUSEBUTTONDOWN:
		if (e.button.button == SDL_BUTTON_LEFT)
		{
			if (!bLeftMBDown)
				bLeftMBDown = true;
		}
		else if (e.button.button == SDL_BUTTON_RIGHT)
		{
			if (!bRightMBDown)
				bRightMBDown = true;
		}
		break;

	case SDL_MOUSEMOTION:
		if (bLeftMBDown)
		{
			if (e.motion.xrel != 0)
				RotateYaw((float)e.motion.xrel * fRotateSensitivity * fDeltaTime);
			if (e.motion.yrel != 0)
				RotatePitch((float)e.motion.yrel * fRotateSensitivity * fDeltaTime);
		}
		else if (bRightMBDown)
		{
			if (e.motion.xrel != 0)
				MoveHorizontal((float)e.motion.xrel * -fRotateSensitivity * fDeltaTime);
			if (e.motion.yrel != 0)
				MoveVertical((float)e.motion.yrel * fRotateSensitivity * fDeltaTime);
		}
		break;

	case SDL_MOUSEBUTTONUP:
		if(bLeftMBDown)
			bLeftMBDown = false;
		if (bRightMBDown)
			bRightMBDown = false;
		break;

	case SDL_MOUSEWHEEL:
		Zoom(e.wheel.y * fZoomSensitivity * fDeltaTime);
		break;
	}
}

void Camera::RotateYaw(const float fRotateRad)
{
	fYaw += fRotateRad;
	fYaw = fmodf(fYaw, PI_2);
	if (fYaw < 0.f)
		fYaw = PI_2 - fYaw;

	UpdateMatView();
}

void Camera::RotatePitch(const float fRotateRad)
{
	fPitch += fRotateRad;

	//clamp to 90 degrees or PI / 2 to stop it going over the top and rotating in the backwards direction
	if (fPitch > PI_HALF)
		fPitch = PI_HALF;
	else if (fPitch < -PI_HALF)
		fPitch = -PI_HALF;

	UpdateMatView();
}

void Camera::Zoom(const float fDistance)
{
	fRadius = glm::max(fRadius - fDistance, .02f);

	UpdateMatView();
}

void Camera::MoveHorizontal(const float fDistance)
{
	glm::vec3 vViewVector = glm::normalize(vTarget - persMatrices.vEyePos);
	glm::vec3 vStrafeVector = glm::normalize(glm::cross(vViewVector, vUp));
	vTarget += vStrafeVector * fDistance;

	UpdateMatView();
}

void Camera::MoveVertical(const float fDistance)
{
	vTarget += vUp * fDistance;
	UpdateMatView();
}

void Camera::UpdateMatView()
{
	persMatrices.vEyePos.x = vTarget.x + fRadius * glm::cos(fPitch) * glm::cos(fYaw);
	persMatrices.vEyePos.y = vTarget.y + fRadius * glm::sin(fPitch);
	persMatrices.vEyePos.z = vTarget.z + fRadius * glm::cos(fPitch) * glm::sin(fYaw);

	persMatrices.matView = glm::lookAt(persMatrices.vEyePos, vTarget, glm::vec3(0.f, 1.f, 0.f));

	//spdlog::info("POS: " + std::to_string(persMatrices.vEyePos.x) + ", " + std::to_string(persMatrices.vEyePos.y) + ", " + std::to_string(persMatrices.vEyePos.z));
	//spdlog::info("TAR: " + std::to_string(vTarget.x) + ", " + std::to_string(vTarget.y) + ", " + std::to_string(vTarget.z));
	//spdlog::info("RAD: " + std::to_string(fRadius));
	//spdlog::warn(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

	UpdateUBO();
}

void Camera::UpdateMatProjection()
{
	persMatrices.matProj = glm::perspective(glm::radians(fFOV), fAspectRatio, fNear, fFar);

	UpdateUBO();
}

void Camera::UpdateUBO()
{
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboPerspec);
	PersMatrices* persMat = (PersMatrices*)glMapNamedBufferRange(uboPerspec, 0, sizeof(PersMatrices), GL_MAP_WRITE_BIT);
	persMat[0] = persMatrices;
	glUnmapNamedBuffer(uboPerspec);
}

void Camera::SetTarget(const glm::vec3 vTarget)
{
	this->vTarget = vTarget;
}

glm::mat4 Camera::GetProjMatrix() const
{
	return persMatrices.matProj;
}

glm::mat4 Camera::GetViewMatrix() const
{
	return persMatrices.matView;
}
