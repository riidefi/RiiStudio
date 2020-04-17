#include "Material.hpp"

#include <core/kpi/Node.hpp>

namespace riistudio::j3d {

const libcube::Texture& Material::getTexture(const std::string& id) const {
  // Assumption: Parent of parent model is a collection with children.
  const kpi::IDocumentNode* parent = getParent();
  assert(parent);
  const kpi::IDocumentNode* collection = static_cast<kpi::IDocumentNode*>(parent->parent);
  assert(collection);

  const auto* textures = collection->getFolder<libcube::Texture>();

  for (std::size_t i = 0; i < textures->size(); ++i) {
    const auto& at = textures->at<libcube::Texture>(i);

    if (at.getName() == id)
      return at;
  }

  throw "Invalid";
}

} // namespace riistudio::j3d
