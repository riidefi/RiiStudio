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
#if !defined(HAS_RUST_TRY)
  try {
#endif // HAS_RUST_TRY
    draw(); // Call down to window manager
#if !defined(HAS_RUST_TRY)
  } catch (std::string err) {
    fprintf(stderr, "Catching unhandled exception: %s\n", err.c_str());
  }
#endif // HAS_RUST_TRY
}

} // namespace riistudio::frontend
