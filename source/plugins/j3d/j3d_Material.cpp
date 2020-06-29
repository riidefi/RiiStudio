#include "Material.hpp"
#include <plugins/gc/Export/Scene.hpp>
#include <core/kpi/Node.hpp>

namespace riistudio::j3d {

const libcube::Texture* Material::getTexture(const std::string& id) const {
  auto* parent = childOf;
  assert(parent);
  auto* grandparent = parent->childOf;
  assert(grandparent);
  const libcube::Scene* pScn = dynamic_cast<const libcube::Scene*>(grandparent);
  assert(pScn);
  return pScn->getTextures().findByName(id);
}

} // namespace riistudio::j3d
