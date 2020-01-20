#pragma once

struct GLFWwindow;

class GL_Window
{
public:
	GL_Window(int width = 1280, int height = 720, const char* pName = "Untitled Window");
	~GL_Window();

	void frameLoop();

	virtual void frameProcess() = 0;
	virtual void frameRender()  = 0;

	void setDropCallback(void(*callback)(GLFWwindow* window, int count, const char** paths));

	GLFWwindow* getGlfwWindow()
	{
		return mpGlfwWindow;
	}

private:
	GLFWwindow* mpGlfwWindow;
};
