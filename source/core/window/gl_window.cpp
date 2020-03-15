#include "gl_window.hpp"

#include <cstdio>
#include <core/3d/gl.hpp>
#ifdef RII_BACKEND_GLFW
#include <vendor/glfw/glfw3.h>
#endif
#include <vendor/imgui/imgui.h>
#include <stdlib.h>

#include <string_view>
#include <vector>

#include <vendor/imgui/impl/imgui_impl_opengl3.h>
#include <vendor/imgui/impl/imgui_impl_glfw.h>


#include <stdint.h>

#ifndef _WIN32
#include <emscripten.h>
#include <vendor/imgui/impl/imgui_impl_sdl.h>

#include <SDL.h>
#include <SDL_opengles2.h>
#include <emscripten/bind.h>

// Emscripten requires to have full control over the main loop. We're going to store our SDL book-keeping variables globally.
// Having a single function that acts as a loop prevents us to store state in the stack of said function. So we need some location for this.
SDL_Window* g_Window = NULL;
SDL_GLContext   g_GLContext = NULL;

#endif

namespace riistudio::core {

static GLWindow* g_AppWindow = nullptr;


#ifdef RII_BACKEND_GLFW
static void handleGlfwError(int err, const char* description)
{
	fprintf(stderr, "GLFW Error: %d: %s\n", err, description);
}

static void handleDrop(GLFWwindow* window, int count, const char** raw_paths)
{
	GLWindow* glwin = reinterpret_cast<GLWindow*>(glfwGetWindowUserPointer(window));
	if (!glwin || !count || !raw_paths) return;

	std::vector<std::string> paths;
	for (int i = 0; i < count; ++i)
		paths.emplace_back(raw_paths[i]);

	glwin->vdrop(paths);
}
#else

void readFile(const int& addr, const size_t& len, std::string path) {
	uint8_t* data = reinterpret_cast<uint8_t*>(addr);
	printf("Data: %p\n", data);

	g_AppWindow->vdropDirect(std::unique_ptr<uint8_t[]>(data), len, path);
}

EMSCRIPTEN_BINDINGS(my_module) {
	emscripten::function("readFile", &readFile);
}
#endif
#ifndef _WIN32
void main_loop(void* arg)
{
	reinterpret_cast<GLWindow*>(arg)->mainLoopInternal();
}
#endif

#ifdef RII_BACKEND_GLFW
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
#endif

	glfwSwapInterval(1); // vsync
	return true;
}
#endif

#ifdef RII_BACKEND_GLFW
namespace {
static const char* glsl_version =
#ifdef _WIN32
"#version 130";
#else
nullptr;
#endif
}
#endif


GLWindow::GLWindow(int width, int height, const char* pName)
{
#ifdef RII_BACKEND_GLFW
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

		gl3wInit();

		// Defer init..
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

		ImGui_ImplGlfw_InitForOpenGL(getGlfwWindow(), true);

		ImGui_ImplOpenGL3_Init(glsl_version);
		return;
	}

	fprintf(stderr, "Failed to initialize GLFW!\n");
	exit(1);
#elif defined(RII_BACKEND_SDL)
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return;
	}

	// For the browser using Emscripten, we are going to use WebGL1 with GL ES2. See the Makefile. for requirement details.
	// It is very likely the generated file won't work in many browsers. Firefox is the only sure bet, but I have successfully
	// run this code on Chrome for Android for example.
	const char* glsl_version = "#version 100";
	//const char* glsl_version = "#version 300 es";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	g_Window = SDL_CreateWindow(pName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
	g_GLContext = SDL_GL_CreateContext(g_Window);


	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	if (!g_GLContext)
	{
		fprintf(stderr, "Failed to initialize WebGL context!\n");
		return;
	}


	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
	// You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
	io.IniFilename = NULL;

	SDL_GL_SetSwapInterval(1); // Enable vsync

	ImGui_ImplSDL2_InitForOpenGL(g_Window, g_GLContext);
#else
#error "No backend selected"
#endif



#ifndef _WIN32
	g_AppWindow = this;
	emscripten_set_main_loop_arg(&main_loop, (void*)this, 0, 0);
#endif

}

void GLWindow::glfwshowMouse()
{
#ifdef RII_BACKEND_GLFW
	glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif
#ifdef RII_BACKEND_SDL
	SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
}
void GLWindow::glfwhideMouse()
{
#ifdef RII_BACKEND_GLFW
	glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif
#ifdef RII_BACKEND_SDL
	SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
}
void GLWindow::setVsync(bool v)
{
#ifdef RII_BACKNED_GLFW
	glfwSwapInterval
#endif
#ifdef RII_BACKEND_SDL
SDL_GL_SetSwapInterval
#endif
		(v ? 1 : 0);
}
GLWindow::~GLWindow()
{
#ifdef RII_BACKEND_GLFW
	glfwDestroyWindow(mGlfwWindow);
	glfwTerminate(); // TODO: Move out
#endif
}

void GLWindow::mainLoopInternal()
{
#ifdef RII_BACKEND_GLFW
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

#endif
#ifdef RII_BACKEND_SDL
	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		// Capture events here, based on io.WantCaptureMouse and io.WantCaptureKeyboard
		switch (event.type)
		{
		case SDL_DROPFILE:
		{
			vdrop({ event.drop.file });
			break;
		}
		default:
			break;
		}
	}



	frameProcess();

	auto& io = ImGui::GetIO();

	SDL_GL_MakeCurrent(g_Window, g_GLContext);
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	// glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	frameRender();
	SDL_GL_SwapWindow(g_Window);

#endif

}

void GLWindow::loop()
{
#ifdef _WIN32
#ifdef RII_BACKEND_GLFW
	while (!glfwWindowShouldClose(mGlfwWindow))
#endif
	{
		mainLoopInternal();
	}
#else
#endif
}

}
