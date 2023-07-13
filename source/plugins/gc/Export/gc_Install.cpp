#include <core/common.h>
#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/Plugins.hpp>

#include "Bone.hpp"
#include "IndexedPolygon.hpp"
#include "Material.hpp"
#include "Texture.hpp"

#include <LibBadUIFramework/RichNameManager.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include "Scene.hpp"

namespace libcube {

kpi::ConstCollectionRange<Texture>
IGCMaterial::getTextureSource(const libcube::Scene& scn) const {
  return scn.getTextures();
}

} // namespace libcube

using namespace libcube;

static ImVec4 Clr(u32 x) {
  return ImVec4{
      static_cast<float>(x >> 16) / 255.0f,
      static_cast<float>((x >> 8) & 0xff) / 255.0f,
      static_cast<float>(x & 0xff) / 255.0f,
      1.0f,
  };
}

void InstallGC() {
  kpi::RichNameManager& rich = kpi::RichNameManager::getInstance();
  rich.addRichName<riistudio::lib3d::Bone>((const char*)ICON_FA_BONE, "Bone",
                                           "", "", Clr(0xFFCA3A))
      .addRichName<riistudio::lib3d::Material>(
          (const char*)ICON_FA_PAINT_BRUSH, "Material", "", "", Clr(0x6A4C93))
      .addRichName<riistudio::lib3d::Texture>(
          (const char*)ICON_FA_IMAGE, "Texture", (const char*)ICON_FA_IMAGES,
          "", Clr(0x8AC926))
      .addRichName<riistudio::lib3d::Polygon>((const char*)ICON_FA_DRAW_POLYGON,
                                              "Polygon")
      .addRichName<riistudio::lib3d::Model>((const char*)ICON_FA_CUBE, "Model",
                                            (const char*)ICON_FA_CUBES, "",
                                            Clr(0x1982C4))
      .addRichName<riistudio::lib3d::Scene>((const char*)ICON_FA_SHAPES,
                                            "Scene");
}
