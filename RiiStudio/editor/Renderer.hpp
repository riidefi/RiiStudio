#pragma once

#include <LibCore/common.h>

#include <Lib3D/interface/i3dmodel.hpp>
#include <LibCore/api/Collection.hpp>

#include <memory>

#include <glm/mat4x4.hpp>

struct SceneState;

class Renderer
{
public:
	Renderer();
	~Renderer();
	void render(u32 width, u32 height, bool& hideCursor);
	void createShaderProgram() { bShadersLoaded = true; }
	void destroyShaders() { bShadersLoaded = false; }

	void prepare(const px::CollectionHost& model, const px::CollectionHost& texture);

private:
	bool bShadersLoaded = false;


	glm::vec3 max{ 0, 0, 0 };
	std::vector<glm::vec3> pos;
	std::vector<u32> idx;
	std::vector<glm::vec3> colors;

	std::unique_ptr<SceneState> mState;

	
	// Mouse
	float mouseSpeed = 0.02f;
	// Input
	bool inCtrl = false;
	int combo_choice_cam = 0;
	// Camera settings
	float speed = 0.0f;
	float cmin = 100.f;
	float cmax = 100000.f;
	float fov = 90.0f;
	// Camera state
	glm::vec3 eye{ 0.0f, 0.0f, 0.0f };
	glm::vec3 direction;
	// Render settings
	bool rend = true;
	bool wireframe = false;
};
