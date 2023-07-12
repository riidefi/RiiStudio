#include "Util.hpp"
#include <core/3d/gl.hpp>
#include <imgui/imgui.h>

namespace librii::glhelper {

bool IsGlWireframeSupported() {
#ifdef RII_NATIVE_GL_WIREFRAME
  return true;
#else
  return false;
#endif
}

void SetGlWireframe(bool wireframe) {
#ifdef RII_NATIVE_GL_WIREFRAME
  if (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

void ClearGlScreen() {
#ifdef RII_GL
  glDepthMask(GL_TRUE);
  const auto bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
  glClearColor(bg.x, bg.y, bg.z, bg.w);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

const char* GetGpuName() {
  static std::string renderer =
      reinterpret_cast<const char*>(glGetString(GL_RENDERER));

  return renderer.c_str();
}

const char* GetGlVersion() {
  static std::string version =
      reinterpret_cast<const char*>(glGetString(GL_VERSION));

  return version.c_str();
}

} // namespace librii::glhelper
