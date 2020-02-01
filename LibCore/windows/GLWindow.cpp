#include "GLWindow.hpp"

#include <cstdio>
#include <ThirdParty/GL/gl3w.h>
#include <ThirdParty/glfw/glfw3.h>
#include <ThirdParty/imgui/imgui.h>
#include <stdlib.h>

#include <string_view>
#include <vector>

static void handleGlfwError(int err, const char* description)
{
	fprintf(stderr, "GLFW Error: %d: %s\n", err, description);
}
static void handleDrop(GLFWwindow* window, int count, const char** raw_paths)
{
	GLWindow* glwin = reinterpret_cast<GLWindow*>(glfwGetWindowUserPointer(window));
	if (!glwin || !count || !raw_paths) return;

	std::vector<std::string_view> paths;
	for (int i = 0; i < count; ++i)
		paths.emplace_back(raw_paths[i]);

	glwin->drop(paths);
}
static bool initWindow(GLFWwindow*& pWin, int width = 1280, int height = 720, const char* pName = "Untitled Window", GLWindow* user=nullptr)
{
	pWin = glfwCreateWindow(width, height, pName, NULL, NULL);
	if (!pWin)
		return false;

	glfwSetWindowUserPointer(pWin, user);
	glfwSetDropCallback(pWin, handleDrop);

	glfwMakeContextCurrent(pWin);
	glfwSwapInterval(1); // vsync
	return true;
}

GLWindow::GLWindow(int width, int height, const char* pName)
{
	glfwSetErrorCallback(handleGlfwError);
	
	if (glfwInit())
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		initWindow(mGlfwWindow, width, height, pName, this);

		if (!gl3wInit())
			return;
	}

	fprintf(stderr, "Failed to initialize GLFW!\n");
	exit(1);
}

GLWindow::~GLWindow()
{
	glfwDestroyWindow(mGlfwWindow);
	glfwTerminate(); // TODO: Move out
}

void GLWindow::loop()
{
	while (!glfwWindowShouldClose(mGlfwWindow))
	{
		glfwPollEvents();

		frameProcess();
	
		int display_w, display_h;
		glfwGetFramebufferSize(mGlfwWindow, &display_w, &display_h);
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
		glfwSwapBuffers(mGlfwWindow);
	}
}
