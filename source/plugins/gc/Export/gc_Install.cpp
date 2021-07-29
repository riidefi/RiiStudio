#include <core/common.h>
#include <core/kpi/Node.hpp>

#include "Bone.hpp"
#include "IndexedPolygon.hpp"
#include "Material.hpp"
#include "Texture.hpp"

#include <core/kpi/RichNameManager.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include "Scene.hpp"

namespace libcube {



kpi::ConstCollectionRange<Texture>
IGCMaterial::getTextureSource(const libcube::Scene& scn) const {
  return scn.getTextures();
}

} // namespace libcube

using namespace libcube;

void InstallGC () {
  auto& installer = *kpi::ApplicationPlugins::getInstance();
  installer.registerParent<libcube::IBoneDelegate, riistudio::lib3d::Bone>()
      .registerParent<libcube::IGCMaterial, riistudio::lib3d::Material>()
      .registerParent<libcube::Texture, riistudio::lib3d::Texture>()
      .registerParent<libcube::IndexedPolygon, riistudio::lib3d::Polygon>();

  kpi::RichNameManager& rich = kpi::RichNameManager::getInstance();
  rich.addRichName<riistudio::lib3d::Bone>((const char*)ICON_FA_BONE, "Bone")
      .addRichName<riistudio::lib3d::Material>((const char*)ICON_FA_PAINT_BRUSH,
                                               "Material")
      .addRichName<riistudio::lib3d::Texture>((const char*)ICON_FA_IMAGE,
                                              "Texture", (const char*)ICON_FA_IMAGES)
      .addRichName<riistudio::lib3d::Polygon>((const char*)ICON_FA_DRAW_POLYGON,
                                              "Polygon")
      .addRichName<riistudio::lib3d::Model>((const char*)ICON_FA_CUBE, "Model",
                                            (const char*)ICON_FA_CUBES)
      .addRichName<riistudio::lib3d::Scene>((const char*)ICON_FA_SHAPES,
                                            "Scene");
}
