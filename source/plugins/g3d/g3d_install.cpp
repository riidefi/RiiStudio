#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/Plugins.hpp>
#include <LibBadUIFramework/RichNameManager.hpp>
#include <core/common.h>
#include <vendor/fa5/IconsFontAwesome5.h>

#include "G3dIo.hpp"
#include "collection.hpp"
#include "model.hpp"

namespace riistudio::g3d {

static ImVec4 Clr(u32 x) {
  return ImVec4{
      static_cast<float>(x >> 16) / 255.0f,
      static_cast<float>((x >> 8) & 0xff) / 255.0f,
      static_cast<float>(x & 0xff) / 255.0f,
      1.0f,
  };
}

void InstallG3d() {
  kpi::RichNameManager& rich = kpi::RichNameManager::getInstance();
  rich.addRichName<g3d::ColorBuffer>((const char*)ICON_FA_BRUSH,
                                     "Vertex Color");
  rich.addRichName<g3d::SRT0>((const char*)ICON_FA_WAVE_SQUARE,
                              "Texture Matrix Animation", "", "",
                              Clr(0xFF595E));
}

} // namespace riistudio::g3d
