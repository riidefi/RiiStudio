#include "ShaderProgram.hpp"


#include <GL/gl3w.h>
#include "GLFW/glfw3.h"

#include <iostream>

void checkShaderErrors(u32 id)
{
	return;
	int success;
	char infoLog[512];
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(id, 512, NULL, infoLog);
		std::cout << "Shader compilation failed: " << infoLog << std::endl;
	}
}

ShaderProgram::ShaderProgram(const char* vtx, const char* frag)
{
    u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vtx, NULL);
    glCompileShader(vertexShader);
    checkShaderErrors(vertexShader);
	// printf(vtx);

    u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag, NULL);
    glCompileShader(fragmentShader);
    checkShaderErrors(fragmentShader);
	// printf(frag);

    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, vertexShader);
    glAttachShader(mShaderProgram, fragmentShader);
    glLinkProgram(mShaderProgram);
    checkShaderErrors(mShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
ShaderProgram::ShaderProgram(const std::string& vtx, const std::string& frag)
    : ShaderProgram(vtx.c_str(), frag.c_str())
{
}
ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(mShaderProgram);
}
