#pragma once

#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include <core/common.h>
#include <core/3d/aabb.hpp>
#include <core/3d/i3dmodel.hpp>

#include <plugins/gc/Export/Bone.hpp>

namespace riistudio::g3d {

struct BoneData
{
	std::string mName = "Untitled Bone";

    // Flags:
        // SRT checks -- recompute
        bool ssc = false;
        // SSC parent - recompute
        bool classicScale = true;
        bool visible = true;
        // display matrix flag not used
        // bb ref recomputed

	u32 mId;
	u32 matrixId;
	u32 flag;

	u32 billboardType;

	glm::vec3 mScaling, mRotation, mTranslation;

	lib3d::AABB mVolume;

	s32 mParent;
	std::vector<s32> mChildren;

	// TODO: userdata

	struct DisplayCommand
	{
		u32 mMaterial;
		u32 mPoly;
		u8 mPrio;

		bool operator==(const DisplayCommand& rhs) const {
			return mMaterial == rhs.mMaterial && mPoly == rhs.mPoly && mPrio == rhs.mPrio;
		}
	};
	std::vector<DisplayCommand> mDisplayCommands;

	std::array<f32, 3*4>
		modelMtx;
	std::array<f32, 3*4>
		inverseModelMtx;

	bool operator==(const BoneData& rhs) const {
		return mName == rhs.mName && ssc == rhs.ssc && classicScale == rhs.classicScale && visible == rhs.visible &&
			mId == rhs.mId && matrixId == rhs.matrixId && flag == rhs.flag && billboardType == rhs.billboardType &&
			mScaling == rhs.mScaling && mRotation == rhs.mRotation && mVolume == rhs.mVolume && mParent == rhs.mParent && mChildren == rhs.mChildren &&
			mDisplayCommands == rhs.mDisplayCommands && modelMtx == rhs.modelMtx && inverseModelMtx == rhs.inverseModelMtx;
	}
};

struct Bone : public libcube::IBoneDelegate, BoneData
{
	std::string getName() const {
		return mName;
	}
	void setName(const std::string& name) override {
		mName = name;
	}
	// std::string getName() const override { return mName; }
	s64 getId() override { return mId; }
	void copy(lib3d::Bone& to) const override
	{
		IBoneDelegate::copy(to);
		Bone* pJoint = dynamic_cast<Bone*>(&to);
		if (pJoint)
		{
			pJoint->mName = mName;
			pJoint->matrixId = matrixId;
			pJoint->flag = flag;

			pJoint->billboardType = billboardType;
			// billboardRefId = pJoint->billboardRefId;
		}

	}
	lib3d::Coverage supportsBoneFeature(lib3d::BoneFeatures f) override
	{
		switch (f)
		{
		case lib3d::BoneFeatures::SRT:
		case lib3d::BoneFeatures::Hierarchy:
		case lib3d::BoneFeatures::StandardBillboards:
		case lib3d::BoneFeatures::AABB:
		case lib3d::BoneFeatures::SegmentScaleCompensation:
		case lib3d::BoneFeatures::ExtendedBillboards:
			return lib3d::Coverage::ReadWrite;

		case lib3d::BoneFeatures::BoundingSphere:
		default:
			return lib3d::Coverage::Unsupported;
		}
	}

	lib3d::SRT3 getSRT() const override {
		return { mScaling, mRotation, mTranslation };
	}
	void setSRT(const lib3d::SRT3& srt) override {
		mScaling = srt.scale;
		mRotation = srt.rotation;
		mTranslation = srt.translation;
	}
	s64 getBoneParent() const override { return mParent; }
	void setBoneParent(s64 id) override { mParent = (u32)id; }
	u64 getNumChildren() const override { return mChildren.size(); }
	s64 getChild(u64 idx) const override { return idx < mChildren.size() ? mChildren[idx] : -1; }
	s64 addChild(s64 child) override { mChildren.push_back((u32)child); return mChildren.size() - 1; }
	s64 setChild(u64 idx, s64 id) override {
		s64 old = -1;
		if (idx < mChildren.size()) { old = mChildren[idx]; mChildren[idx] = (u32)id; }
		return old;
	}
	lib3d::AABB getAABB() const override { return { mVolume.min, mVolume.max }; }
	void setAABB(const lib3d::AABB& aabb) override { mVolume = { aabb.min, aabb.max }; }
	float getBoundingRadius() const override { return 0.0f; }
	void setBoundingRadius(float r) override { }
	bool getSSC() const override { return ssc; }
	void setSSC(bool b) override { ssc = b; }


	Billboard getBillboard() const override
	{
		// TODO:
		// switch (billboardType)
		{
			return Billboard::None;
		}
	}
	void setBillboard(Billboard b) override
	{
		//	switch (b)
		//	{
		//	case Billboard::None:
		//		bbMtxType = MatrixType::Standard;
		//		break;
		//	case Billboard::J3D_XY:
		//		bbMtxType = MatrixType::Billboard;
		//		break;
		//	case Billboard::J3D_Y:
		//		bbMtxType = MatrixType::BillboardY;
		//		return;
		//	default:
		//		break;
		//	}
	}

	u64 getNumDisplays() const override
	{
		return mDisplayCommands.size();
	}
	Display getDisplay(u64 idx) const override
	{
		const auto& dc = mDisplayCommands[idx];

		return Display{ dc.mMaterial, dc.mPoly, dc.mPrio };
	}

	void addDisplay(const Display& d) override {
		mDisplayCommands.push_back({ d.matId, d.polyId, d.prio });
	}
	bool operator==(const Bone& rhs) const {
		return static_cast<const BoneData&>(*this) == rhs;
	}
};


}
