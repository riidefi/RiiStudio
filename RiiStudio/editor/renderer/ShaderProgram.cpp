#include "ShaderProgram.hpp"


#include <LibCore/gl.hpp>

#include <iostream>

bool checkShaderErrors(u32 id)
{
	return true;
	int success;
	char infoLog[512];
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(id, 512, NULL, infoLog);
		printf("Shader compilation failed: %s\n", infoLog);
	}

	return success;
}

ShaderProgram::ShaderProgram(const char* vtx, const char* frag)
{
    u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vtx, NULL);
    glCompileShader(vertexShader);
    if (!checkShaderErrors(vertexShader))
		printf("%s\n", vtx);

    u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag, NULL);
    glCompileShader(fragmentShader);
    if (!checkShaderErrors(fragmentShader))
		printf("%s\n", frag);

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
#ifndef RII_PLATFORM_EMSCRIPTEN
    glDeleteProgram(mShaderProgram);
#endif
}
