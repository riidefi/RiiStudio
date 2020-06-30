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

kpi::DecentralizedInstaller Installer([](kpi::ApplicationPlugins& installer) {
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
});

kpi::ConstCollectionRange<Texture>
IGCMaterial::getTextureSource() const {
  auto* parent = childOf;
  assert(parent);
  auto* grandparent = parent->childOf;
  assert(grandparent);
  const libcube::Scene* pScn = dynamic_cast<const libcube::Scene*>(grandparent);
  assert(pScn);
  return pScn->getTextures();
}

} // namespace libcube
