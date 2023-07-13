#include <core/common.h>
#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/Plugins.hpp>

#include "Bone.hpp"
#include "IndexedPolygon.hpp"
#include "Material.hpp"
#include "Texture.hpp"

#include <vendor/fa5/IconsFontAwesome5.h>

#include "Scene.hpp"

namespace libcube {

kpi::ConstCollectionRange<Texture>
IGCMaterial::getTextureSource(const libcube::Scene& scn) const {
  return scn.getTextures();
}

} // namespace libcube

