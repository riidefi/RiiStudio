#include <frontend/widgets/ReflectedPropertyGrid.hpp>

#include "BtkEditor.hpp"

namespace riistudio::frontend {

void BtkEditorPropertyGrid::Draw(librii::j3d::BTK& btk) {
  auto s = btk.debugDump();
  ImGui::Text("%s", s.c_str());
}

} // namespace riistudio::frontend
