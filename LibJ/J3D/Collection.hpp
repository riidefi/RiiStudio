#pragma once

#include "Model.hpp"
#include "Texture.hpp"

#include <LibCore/api/Node.hpp>


namespace libcube { namespace jsystem {

//! Represents the state of a J3D model and textures (BMD, BDL) as well as animations.
//! Unlike the binary equivalents, per-material texture settings have been moved to samplers in materials.
//!
class J3DCollection final : public lib3d::Scene
{
	friend class J3DCollectionSpawner;
public:
	PX_TYPE_INFO("J3D Scene", "j3d_scene", "J::J3DCollection");

	
	J3DCollection()
	{
		acceptChild(PX_GET_TID(J3DModel));
		acceptChild(PX_GET_TID(Texture));
	}

	PX_COLLECTION_FOLDER_GETTER(getModels, J3DModel);
	PX_COLLECTION_FOLDER_GETTER(getTextures, Texture);

	bool bdl = false;
};

} } // namespace libcube::jsystem
