#pragma once

#include "essential_functions.hpp"
#include <LibCube/Common/BoundBox.hpp>
#include <LibCube/Export/GCCollection.hpp>

namespace libcube { namespace pikmin1 {

struct Joint : public IBoneDelegate
{
	constexpr static const char name[] = "Joint";
	constexpr static const char TypeName[] = "pikmin1joint";

	u32 mID; // Set by loader
	std::vector<int> mChildren;
	std::string mName;

	// Real fields
	u32 m_parentIndex;
	bool m_useVolume;
	bool m_foundLightGroup;
	AABB m_boundingBox;
	f32 m_volumeRadius;
	glm::vec3 m_scale;
	glm::vec3 m_rotation;
	glm::vec3 m_translation;
	struct MatPoly
	{
		u16 m_index; // I think this is an index of sorts, not sure what though
		u16 m_unk2;
	};
	std::vector<MatPoly> m_matpolys;

	Joint() = default;
	~Joint() = default;

	static void onRead(oishii::BinaryReader& bReader, Joint& context)
	{
		// Known to be -1 on first iteration (null)
		context.m_parentIndex = bReader.read<u32>();

		const u16 usingIdentifier = static_cast<u16>(bReader.read<u32>());
		// Oddly, only assigned in plugTexConv when volume_min or volume_max has been found
		context.m_useVolume = usingIdentifier != 0;
		// context: plugTexConv assigns this variable on reading dmd file when "light" is found
		context.m_foundLightGroup = (usingIdentifier & 0x4000) != 0;
		// These have been confirmed to be correct
		context.m_boundingBox << bReader;
		// Always 0, when writing is always set to 0
		context.m_volumeRadius = bReader.read<f32>();
		read(bReader, context.m_scale);
		read(bReader, context.m_rotation);
		read(bReader, context.m_translation);

		context.m_matpolys.resize(bReader.read<u32>());
		for (auto& matpoly : context.m_matpolys)
		{
			matpoly.m_index = bReader.read<u16>();
			matpoly.m_unk2 = bReader.read<u16>();
		}
	}

	Billboard getBillboard() const override
	{
		return Billboard::None;
	}
	void setBillboard(Billboard) override {}

	std::string getName() override { return mName; }
	s64 getId() override { return mID; }
	
	virtual lib3d::Coverage supportsBoneFeature(lib3d::BoneFeatures f) override
	{
		switch (f)
		{
		case lib3d::BoneFeatures::SRT:
		case lib3d::BoneFeatures::Hierarchy:
		case lib3d::BoneFeatures::AABB:
		case lib3d::BoneFeatures::BoundingSphere:
			return lib3d::Coverage::ReadWrite;
		case lib3d::BoneFeatures::SegmentScaleCompensation:
		case lib3d::BoneFeatures::StandardBillboards:
		default:
			return lib3d::Coverage::Unsupported;

		}
	}

	lib3d::SRT3 getSRT() const override
	{
		return { m_scale, m_rotation, m_translation };
	}
	void setSRT(const lib3d::SRT3& srt) override
	{
		m_scale = srt.scale;
		m_rotation = srt.rotation;
		m_translation = srt.translation;
	}

	s64 getParent() const override { return m_parentIndex; }
	void setParent(s64 id) override { m_parentIndex = id; }
	u64 getNumChildren() const override { return mChildren.size(); }
	s64 getChild(u64 idx) const override { return mChildren[idx]; }
	s64 addChild(s64 child) override { mChildren.push_back(child);  return mChildren.size() - 1; }
	s64 setChild(u64 idx, s64 id) override { int old = mChildren[idx]; mChildren[idx] = id; return old; }
	
	lib3d::AABB getAABB() const override
	{
		return { m_boundingBox.m_minBounds, m_boundingBox.m_maxBounds };
	}
	void setAABB(const lib3d::AABB& v) override
	{
		m_boundingBox.m_minBounds = v.min;
		m_boundingBox.m_maxBounds = v.max;
	}

	virtual float getBoundingRadius() const override
	{
		return m_volumeRadius;
	}
	virtual void setBoundingRadius(float v) override
	{
		m_volumeRadius = v;
	}

	virtual bool getSSC() const override { return false; }
	virtual void setSSC(bool) override {}
};

inline void operator<<(Joint& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<Joint, oishii::Direct, false>(context);
}

} }
