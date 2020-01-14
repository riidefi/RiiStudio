#pragma once

#include "Collection.hpp"
#include <string>

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

struct ExampleXF : public pl::TransformStack::XForm
{
	ExampleXF()
	{
		name = {
			"Triangle Stripping",
			"j3dcollection.tristrip",
			"tristrip"
		};
		addParam(
			pl::RichName{ "Cache Size", "cache_sz", "cache_size" },
			"1024"
			);
		addParam(
			pl::RichName{ "Minimum Strip Length", "min_strip", "minimum_strip_length" },
			"2"
		);
		addParam(
			pl::RichName{ "Push Cache Hits", "push_cache", "push_cache_hits" },
			true
		);
		addParam(pl::TransformStack::Param(
			{ "Algorithm", "algo", "algorithm" },
			{
				{ "SGI", "sgi", "sgi" },
				{ "MStrip", "mstrip", "mstrip" },
				{ "STRIPE", "stripe", "stripe"}
			},
			0
		));
	}
	void perform(const pl::TransformStack::XFormContext& ctx) const override
	{
		printf("Triangle stripping!\n");
		const auto cache_sz = std::stoi(ctx.getString("cache_sz"));
		const auto min_strip = std::stoi(ctx.getString("min_strip"));
		const auto push_cache = ctx.getFlag("push_cache");
		const auto algo = ctx.getEnumExposedCStr("algo");
		printf("cache_sz: %i, min_strip: %i, push_cache: %s, algo: %s\n", cache_sz, min_strip, push_cache ? "true" : "false", algo);
	}
};

J3DCollection::J3DCollection()
{
	internal = std::make_unique<Internal>();

	//	registerInterface<pl::ITextureList>();
	//	registerInterface<GCCollection>();
	//	registerInterface<pl::TransformStack>();
	mXfStack.mStack.push_back(std::make_unique<ExampleXF>());
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
