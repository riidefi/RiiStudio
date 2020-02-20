#include "GLWindow.hpp"

#include <cstdio>
#include <LibCore/gl.hpp>
#include <ThirdParty/glfw/glfw3.h>
#include <ThirdParty/imgui/imgui.h>
#include <stdlib.h>

#include <string_view>
#include <vector>

#ifndef _WIN32
#include <emscripten.h>
#endif

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

#ifndef _WIN32
void main_loop(void* arg)
{
	reinterpret_cast<GLWindow*>(arg)->mainLoopInternal();
}
#endif


static bool initWindow(GLFWwindow*& pWin, int width = 1280, int height = 720, const char* pName = "Untitled Window", GLWindow* user=nullptr)
{
	pWin = glfwCreateWindow(width, height, pName, NULL, NULL);
	if (!pWin)
		return false;

	glfwSetWindowUserPointer(pWin, user);
	glfwSetDropCallback(pWin, handleDrop);

	glfwMakeContextCurrent(pWin);

#ifndef _WIN32
	emscripten_set_main_loop_arg(&main_loop, (void*)user, 0, 0);
	printf("Hello world\n");
#endif

	glfwSwapInterval(1); // vsync
	return true;
}


GLWindow::GLWindow(int width, int height, const char* pName)
{
	glfwSetErrorCallback(handleGlfwError);

	if (glfwInit())
	{
#ifndef _WIN32
		// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

#else
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	
		glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, true );
#endif

		initWindow(mGlfwWindow, width, height, pName, this);

#ifdef _WIN32
		if (!gl3wInit())
			return;
#else
		return;
#endif
	}

	fprintf(stderr, "Failed to initialize GLFW!\n");
	exit(1);
}

void GLWindow::glfwshowMouse()
{
	glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
void GLWindow::glfwhideMouse()
{
	glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void GLWindow::setVsync(bool v)
{
	glfwSwapInterval(v ? 1 : 0);
}
GLWindow::~GLWindow()
{
	glfwDestroyWindow(mGlfwWindow);
	glfwTerminate(); // TODO: Move out
}

void GLWindow::mainLoopInternal()
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

void GLWindow::loop()
{
#ifdef _WIN32
	while (!glfwWindowShouldClose(mGlfwWindow))
	{
		mainLoopInternal();
	}
#else
#endif
}
