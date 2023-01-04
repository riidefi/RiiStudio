#include <imcxx/Widgets.hpp>
#include <numeric> // roundf

namespace riistudio {

void DrawFps() {
  const auto& io = ImGui::GetIO();
#ifndef NDEBUG
  ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate,
              io.Framerate ? 1000.0f / io.Framerate : 0.0f);
#else
  ImGui::Text("%u fps", static_cast<u32>(roundf(io.Framerate)));
#endif
}

} // namespace riistudio
