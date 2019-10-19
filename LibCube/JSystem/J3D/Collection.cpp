#pragma once

#include "Collection.hpp"

namespace libcube::jsystem {

using gcdel = GCCollection::IMaterialDelegate;


struct J3DMaterialDelegate
	: public GCCollection::IMaterialDelegate,
	public gcdel::Property<gcdel::GenInfoCounts>
{
	~J3DMaterialDelegate() override = default;
	J3DMaterialDelegate(J3DModel::Material& mat)
		: mMat(mat), cullMode(mMat.cullMode), zCompLoc(mat.earlyZComparison)
	{}

	gcdel::RefProperty<gx::CullMode> cullMode;
	gcdel::Property<gx::CullMode>* getCullMode() override
	{
		return &cullMode;
	}
	gcdel::RefProperty<bool> zCompLoc;
	gcdel::Property<bool>* getZCompLoc() override
	{
		return &zCompLoc;
	}

	GenInfoCounts get() override
	{
		return {
			mMat.info.nColorChannel,
			mMat.info.nTexGen,
			mMat.info.nTevStage,
			mMat.info.indirect ? mMat.info.nInd : u8(0)
		};
	}
	void set(GenInfoCounts c)
	{
		mMat.info.nColorChannel = c.colorChan;
		mMat.info.nTexGen = c.texGen;
		mMat.info.nTevStage = c.tevStage;
		mMat.info.indirect = c.indStage;
		mMat.info.nInd = c.indStage;
	}
	gcdel::Property<GenInfoCounts>* getGenInfo() override {
		return static_cast<gcdel::Property<GenInfoCounts>*>(this);
	}

	SupportRegistration getRegistration() const override
	{
		return SupportRegistration {
			PropSupport::ReadWrite,
			PropSupport::ReadWrite,
			PropSupport::Unsupported,
			PropSupport::ReadWrite
		};
	}

	J3DModel::Material& mMat;
};

std::unique_ptr<GCCollection::IMaterialDelegate> J3DCollection::getMaterialDelegate(u32 idx)
{
	assert(idx < mModel.mMaterials.size());
	
	if (idx < mModel.mMaterials.size())
		return std::make_unique<J3DMaterialDelegate>(mModel.mMaterials[idx]);

	return nullptr;
}

} // namespace libcube::jsystem
