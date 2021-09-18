#include "applet.hpp"
#include "Fonts.hpp"
#include <imgui/imgui.h>
#include <vendor/ImGuizmo.h>

namespace riistudio::frontend {

Applet::Applet(const std::string& name) : plate::Platform(1280, 720, name) {
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  if (!loadFonts()) {
    fprintf(stderr, "Failed to load fonts");
  }

#ifdef RII_BACKEND_GLFW
  mpGlfwWindow = (GLFWwindow*)getPlatformWindow();
#endif
}

Applet::~Applet() {}

void Applet::rootDraw() {}
void Applet::rootCalc() {
  ImGuizmo::BeginFrame();
  ImGuizmo::Enable(true);
  detachClosedChildren();
  draw(); // Call down to window manager
}

} // namespace riistudio::frontend
