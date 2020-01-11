#pragma once

#include "Collection.hpp"

namespace libcube::jsystem {

using gcdel = GCCollection::IMaterialDelegate;


struct J3DMaterialDelegate : public GCCollection::IMaterialDelegate
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
}

GCCollection::IMaterialDelegate& J3DCollection::getMaterialDelegate(u32 idx)
{
	assert(idx < mModel.mMaterials.size() && internal);

	return internal->mMaterialDelegates[idx];
}

} // namespace libcube::jsystem
