#pragma once

#include "essential_functions.hpp"
#include <LibCube/Common/BoundBox.hpp>

namespace libcube { namespace pikmin1 {

struct Joint
{
	constexpr static const char name[] = "Joint";

	u32 m_index;
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
		context.m_index = bReader.read<u32>();

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
};

inline void operator<<(Joint& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<Joint, oishii::Direct, false>(context);
}

} }
