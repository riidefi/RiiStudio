#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct Joint
{
	constexpr static const char name[] = "Joint";

	u32 m_unk1;
	bool m_usingBillBoards;
	bool m_unk3;
	glm::vec3 m_boundsMin;
	glm::vec3 m_boundsMax;
	f32 m_volumeRadius;
	glm::vec3 m_scale;
	glm::vec3 m_rotation;
	glm::vec3 m_translation;
	struct MatPoly
	{
		u16 m_index; // I think this is an index of sorts, not sure what though. :/
		u16 m_unk2;
	};
	std::vector<MatPoly> m_matpolys;

	Joint() = default;
	~Joint() = default;

	static void onRead(oishii::BinaryReader& bReader, Joint& context)
	{
		context.m_unk1 = bReader.read<u32>();

		const u16 unk1 = static_cast<u16>(bReader.read<u32>());
		context.m_usingBillBoards = unk1 != 0;
		context.m_unk3 = (unk1 & 0x4000) != 0;

		read(bReader, context.m_boundsMin);
		read(bReader, context.m_boundsMax);
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
