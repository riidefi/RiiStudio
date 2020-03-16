#include "applet.hpp"
#include <imgui/imgui.h>
#include <imgui/impl/imgui_impl_glfw.h>
#include <imgui/impl/imgui_impl_opengl3.h>
#include <fa5/IconsFontAwesome5.h>

#include <vendor/imgui/impl/imgui_impl_sdl.h>


#ifdef RII_BACKEND_SDL
struct SDL_Window;
extern SDL_Window* g_Window;
#endif

namespace riistudio::core {


static bool loadFonts()
{
    ImGuiIO& io = ImGui::GetIO();
	ImFontConfig fontcfg;
	fontcfg.OversampleH = 8;
	fontcfg.OversampleV = 8;

#ifdef BUILD_DIST
#define FONT_DIR "./fonts/"
#else
#define FONT_DIR "../../fonts/"
#endif

	io.Fonts->AddFontFromFileTTF(FONT_DIR "Roboto-Medium.ttf", 14.0f, &fontcfg);

	// Taken from example
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF(FONT_DIR FONT_ICON_FILE_NAME_FAS, 14.0f, &icons_config, icons_ranges);

#undef FONT_DIR
	return true;
}


Applet::Applet(const char* name)
	: GLWindow(1280, 720, name)
{
	
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	if (!loadFonts())
	{
		fprintf(stderr, "Failed to load fonts");
	}
#ifdef RII_BACKEND_GLFW
	mpGlfwWindow = getGlfwWindow();
#endif
}

Applet::~Applet()
{
	ImGui_ImplOpenGL3_Shutdown();
#ifdef RII_BACKEND_GLFW
	ImGui_ImplGlfw_Shutdown();
#else
	ImGui_ImplSDL2_Shutdown();
#endif
	ImGui::DestroyContext();
}

void Applet::frameRender()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
#ifdef DEBUG
static bool demo = true;
#endif
void Applet::frameProcess()
{
	detachClosedChildren();

	ImGui_ImplOpenGL3_NewFrame();
#ifdef RII_BACKEND_GLFW
	ImGui_ImplGlfw_NewFrame();
#else
	ImGui_ImplSDL2_NewFrame(g_Window);
#endif
	ImGui::NewFrame();

	ImGui::GetIO().FontGlobalScale = .8;
#ifdef DEBUG
	ImGui::ShowDemoWindow(&demo);
#endif
    draw(); // Call down to window manager
    

	ImGui::Render();
}

}
