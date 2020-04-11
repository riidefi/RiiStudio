#include "material.hpp"

#include <core/kpi/Node.hpp>

namespace riistudio::g3d {
const libcube::Texture& Material::getTexture(const std::string& id) const
{
    const auto* textures = getTextureSource();

	for (std::size_t i = 0; i < textures->size(); ++i) {
        const auto& at = textures->at<libcube::Texture>(i);
    
		if (at.getName() == id) return at;
    }
	
    throw "Invalid";
}

}
