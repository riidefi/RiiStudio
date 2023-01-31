#include "Material.hpp"
#include <LibBadUIFramework/Plugins.hpp>
#include <plugins/gc/Export/Scene.hpp>

namespace riistudio::j3d {

const libcube::Texture* Material::getTexture(const libcube::Scene& scene,
                                             const std::string& id) const {
  return scene.getTextures().findByName(id);
}

} // namespace riistudio::j3d
