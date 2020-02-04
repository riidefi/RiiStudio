#pragma once

#include <LibCore/common.h>

class Renderer
{
public:
	Renderer();
	~Renderer();
	void render();
	void createShaderProgram();
	void destroyShaders();

private:
	u32 mShaderProgram = 0;
	bool bShadersLoaded = false;
	u32 VBO, VAO, EBO = 0;
};
