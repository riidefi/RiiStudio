#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>

#include <LibRiiEditor/pluginapi/FileStateSpawner.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/XFormStack.hpp>
#include "Model.hpp"

#include <LibCube/Export/GCCollection.hpp>

namespace libcube { namespace jsystem {

//! Represents the state of a J3D model and textures (BMD, BDL) as well as animations.
//! Unlike the binary equivalents, per-material texture settings have been moved to samplers in materials.
//!
class J3DCollection : public pl::FileState, public pl::ITextureList, public GCCollection
{
	friend class J3DCollectionSpawner;
public:
	/*std::vector<std::unique_ptr<*/J3DModel/*>>*/ mModel/*s*/;

	J3DCollection();
	~J3DCollection();

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

	// GCCollection
	u32 getNumMaterials() const override { return static_cast<u32>(mModel.mMaterials.size()); }
	u32 getNumBones() const override { return static_cast<u32>(mModel.mJoints.size()); }
	int addBone() override { auto out = mModel.mJoints.size(); mModel.mJoints.emplace_back(); return out; }
	int addMaterial() override { auto out = mModel.mMaterials.size(); mModel.mMaterials.emplace_back(); return out; }
	
	GCCollection::IMaterialDelegate& getMaterial(u32 idx) override;
	GCCollection::IBoneDelegate& getBone(u32 idx) override;

private:
	pl::TransformStack mXfStack;
public:
	bool bdl = false;
};

struct J3DCollectionSpawner : public pl::FileStateSpawner
{
	~J3DCollectionSpawner() override = default;
	J3DCollectionSpawner()
	{
		mId = { "J3D Collection", "j3dcollection", "j3d_collection" };
		addMirror("j3dcollection", "gc_collection", pl::computeTranslation<J3DCollection, GCCollection>());
		// We don't actually need to inherit the class. A parent will implement this.
		addMirror("j3dcollection", "transform_stack", offsetof(J3DCollection, mXfStack));
	}

	std::unique_ptr<pl::FileState> spawn() const override
	{
		return std::make_unique<J3DCollection>();
	}
	std::unique_ptr<FileStateSpawner> clone() const override
	{
		return std::make_unique<J3DCollectionSpawner>(*this);
	}
};

} } // namespace libcube::jsystem
