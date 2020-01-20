#include "GL_Window.hpp"

#include <cstdio>
#include <ThirdParty/GL/gl3w.h>
#include <ThirdParty/GLFW/glfw3.h>
#include <ThirdParty/imgui/imgui.h>
#include <stdlib.h>

// Loosely based on Dear ImGui impl demo code.

static void handleGlfwError(int err, const char* description)
{
	fprintf(stderr, "GLFW Error: %d: %s\n", err, description);
}

static bool initWindow(GLFWwindow** pWin, int width = 1280, int height = 720, const char* pName = "Untitled Window")
{
	if (pWin == NULL)
		return false;

	*pWin = glfwCreateWindow(width, height, pName, NULL, NULL);
	if (*pWin == NULL)
		return false;

	glfwMakeContextCurrent(*pWin);
	glfwSwapInterval(1); // vsync
}

void GL_Window::setDropCallback(void(*callback)(GLFWwindow* window, int count, const char** paths))
{
	glfwSetDropCallback(mpGlfwWindow, callback);
}

GL_Window::GL_Window(int width, int height, const char* pName)
{
	glfwSetErrorCallback(handleGlfwError);
	
	if (glfwInit())
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		initWindow(&mpGlfwWindow, width, height, pName);

		if (!gl3wInit())
			return;
	}

	fprintf(stderr, "Failed to initialize GLFW!\n");
	exit(1);
}

GL_Window::~GL_Window()
{
	glfwDestroyWindow(mpGlfwWindow);
	glfwTerminate();
}

void GL_Window::frameLoop()
{
	while (!glfwWindowShouldClose(mpGlfwWindow))
	{
		glfwPollEvents();

		frameProcess();
	
		int display_w, display_h;
		glfwGetFramebufferSize(mpGlfwWindow, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		//glClearColor(window.clear_color.x, window.clear_color.y, window.clear_color.z, window.clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		frameRender();

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
		glfwSwapBuffers(mpGlfwWindow);
	}
}
