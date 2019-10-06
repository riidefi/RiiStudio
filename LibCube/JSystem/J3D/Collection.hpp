#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>

namespace libcube { namespace jsystem {

//! Represents the state of a J3D model and textures (BMD, BDL) as well as animations.
//! Unlike the binary equivalents, per-material texture settings have been moved to samplers in materials.
//!
struct J3DCollection : public pl::FileState, public pl::ITextureList
{


	//
	// Interfaces
	//

	// ITextureList
	u32 getNumTex() const override
	{
		return 0;
	}
	std::string getNameAt(int idx) const override
	{
		return "TODO";
	}


	//
	// Construction/Destruction
	//

	~J3DCollection() override = default;
	J3DCollection()
	{
		// Register interfaces
		mInterfaces.push_back(static_cast<pl::ITextureList*>(this));
	}
};

} } // namespace libcube::jsystem
