#include "Applet.hpp"
#include <imgui/imgui.h>
#include <imgui/impl/imgui_impl_glfw.h>
#include <imgui/impl/imgui_impl_opengl3.h>
#include <fa5/IconsFontAwesome5.h>

// Parts taken from ImGui impl example.

namespace {

// GL 3.0 + GLSL 130
const char* glsl_version = "#version 130";

}

static bool loadFonts()
{
	ImGuiIO& io = ImGui::GetIO();

	ImFontConfig fontcfg;
	fontcfg.OversampleH = 8;
	fontcfg.OversampleV = 8;

	io.Fonts->AddFontFromFileTTF("../fonts/Roboto-Medium.ttf", 14.0f, &fontcfg);

	// Taken from example
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 14.0f, &icons_config, icons_ranges);

	return true;
}

Applet::Applet(const char* name)
	: GL_Window(1280, 720, name)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	if (!loadFonts())
	{
		fprintf(stderr, "Failed to load fonts");
	}

	ImGui_ImplOpenGL3_Init(glsl_version);
	ImGui_ImplGlfw_InitForOpenGL(getGlfwWindow(), true);
}

Applet::~Applet()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Applet::frameRender()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Applet::frameProcess()
{
	processWindowQueue();
	// The queue/vector should not be modified here

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	drawRoot();
	// ImGui::ShowDemoWindow();

	{
		WindowContext window_context = makeWindowContext();

		drawWindows(&window_context);
	}

	ImGui::Render();
}
