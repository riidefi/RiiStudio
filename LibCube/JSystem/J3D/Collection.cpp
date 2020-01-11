#pragma once

#include "Collection.hpp"

namespace libcube::jsystem {

using gcdel = GCCollection::IMaterialDelegate;

struct J3DBoneDelegate final : public GCCollection::IBoneDelegate
{
	~J3DBoneDelegate() override = default;
	J3DBoneDelegate(J3DModel::Joint& joint, u32)
		: mJoint(joint)
	{
		using b = GCCollection::BoneFeatures;
		setSupport(b::SRT, Coverage::ReadWrite);
		setSupport(b::Hierarchy, Coverage::Read);
		setSupport(b::StandardBillboards, Coverage::ReadWrite);
		setSupport(b::ExtendedBillboards, Coverage::Unsupported);
		setSupport(b::AABB, Coverage::ReadWrite);
		setSupport(b::AABB, Coverage::ReadWrite);
		setSupport(b::SegmentScaleCompensation, Coverage::ReadWrite);
	}

	const char* getNameCStr() override
	{
		return mJoint.id.c_str();
	}
	
	SRT3 getSRT() const override
	{
		return { mJoint.scale, mJoint.rotate, mJoint.translate };
	}
	void setSRT(const SRT3& srt) override
	{
		mJoint.scale = srt.scale;
		mJoint.rotate = srt.rotation;
		mJoint.translate = srt.translation;
	}

	std::string getParent() const override
	{
		return mJoint.parent;
	}
	std::vector<std::string> getChildren() const override
	{
		return mJoint.children;
	}

	Billboard getBillboard() const override
	{
		switch (mJoint.bbMtxType)
		{
		case J3DModel::Joint::MatrixType::Billboard:
			return Billboard::J3D_XY;
		case J3DModel::Joint::MatrixType::BillboardY:
			return Billboard::J3D_Y;
		case J3DModel::Joint::MatrixType::Standard:
			return Billboard::None;
		default:
			assert(!"Invalid bb mode");
			return Billboard::None;
		}
	}
	void setBillboard(Billboard b) override
	{
		switch (b)
		{
		case Billboard::J3D_XY:
			mJoint.bbMtxType = J3DModel::Joint::MatrixType::Billboard;
			break;
		case Billboard::J3D_Y:
			mJoint.bbMtxType = J3DModel::Joint::MatrixType::BillboardY;
			break;
		case Billboard::None:
			mJoint.bbMtxType = J3DModel::Joint::MatrixType::Standard;
			break;
		default:
			assert(!"Invalid bb mode");
			mJoint.bbMtxType = J3DModel::Joint::MatrixType::Standard;
			break;
		}
	}

	AABB getAABB() const override
	{
		return mJoint.boundingBox;
	}
	void setAABB(const AABB& v) override
	{
		mJoint.boundingBox = v;
	}

	float getBoundingRadius() const override
	{
		return mJoint.boundingSphereRadius;
	}
	void setBoundingRadius(float v) override
	{
		mJoint.boundingSphereRadius = v;
	}

	bool getSSC() const override
	{
		return mJoint.mayaSSC;
	}
	void setSSC(bool b) override
	{
		mJoint.mayaSSC = b;
	}
	J3DModel::Joint mJoint;
};

struct J3DMaterialDelegate final : public GCCollection::IMaterialDelegate
{
	~J3DMaterialDelegate() override = default;
	J3DMaterialDelegate(J3DModel::Material& mat, u32 i)
		: mMat(mat), idx(i)
	{
		using P = PropertySupport;
		support.setSupport(P::Feature::CullMode, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::ZCompareLoc, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::ZCompare, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::GenInfo, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::MatAmbColor, P::Coverage::ReadWrite);
	}

	gx::CullMode getCullMode() const override
	{
		return mMat.cullMode;
	}
	void setCullMode(gx::CullMode c) override
	{
		mMat.cullMode = c;
	}
	bool getZCompLoc() const override
	{
		return mMat.earlyZComparison;
	}
	void setZCompLoc(bool b) override
	{
		mMat.earlyZComparison = b;
	}

	GenInfoCounts getGenInfo() const override
	{
		return {
			mMat.info.nColorChannel,
			mMat.info.nTexGen,
			mMat.info.nTevStage,
			mMat.info.indirect ? mMat.info.nInd : u8(0)
		};
	}
	void setGenInfo(const GenInfoCounts& c)
	{
		mMat.info.nColorChannel = c.colorChan;
		mMat.info.nTexGen = c.texGen;
		mMat.info.nTevStage = c.tevStage;
		mMat.info.indirect = c.indStage;
		mMat.info.nInd = c.indStage;
	}


	gx::Color getMatColor(u32 idx) const override
	{
		if (idx < 2)
			return mMat.matColors[idx];

		return {};
	}
	virtual gx::Color getAmbColor(u32 idx) const override
	{
		if (idx < 2)
			return mMat.ambColors[idx];

		return {};
	}
	void setMatColor(u32 idx, gx::Color v) override
	{
		if (idx < 2)
			mMat.matColors[idx] = v;
	}
	void setAmbColor(u32 idx, gx::Color v) override
	{
		if (idx < 2)
			mMat.ambColors[idx] = v;
	}

	const char* getNameCStr() override
	{
		return mMat.id.c_str();
	}
	void* getRaw() override
	{
		return (void*)idx;
	}
	J3DModel::Material& mMat;
	u32 idx;
};

struct J3DCollection::Internal
{
	std::vector<J3DMaterialDelegate> mMaterialDelegates;
	std::vector<J3DBoneDelegate> mBoneDelegates;
};

J3DCollection::J3DCollection()
{
	internal = std::make_unique<Internal>();

	registerInterface<pl::ITextureList>();
	registerInterface<GCCollection>();

	update();
}
J3DCollection::~J3DCollection()
{
}
void J3DCollection::update()
{
	// FIXME: Slow
	internal->mMaterialDelegates.clear();
	internal->mMaterialDelegates.reserve(mModel.mMaterials.size());
	u32 i = 0;
	for (auto& mat : mModel.mMaterials)
	{
		internal->mMaterialDelegates.emplace_back(mat, i);
		i++;
	}
	internal->mBoneDelegates.clear();
	internal->mBoneDelegates.reserve(mModel.mJoints.size());
	i = 0;
	for (auto& bone : mModel.mJoints)
	{
		internal->mBoneDelegates.emplace_back(bone, i);
		i++;
	}
}

GCCollection::IMaterialDelegate& J3DCollection::getMaterialDelegate(u32 idx)
{
	assert(idx < mModel.mMaterials.size() && internal);

	return internal->mMaterialDelegates[idx];
}
GCCollection::IBoneDelegate& J3DCollection::getBoneDelegate(u32 idx)
{
	assert(idx < mModel.mJoints.size() && internal);

	return internal->mBoneDelegates[idx];
}


} // namespace libcube::jsystem
