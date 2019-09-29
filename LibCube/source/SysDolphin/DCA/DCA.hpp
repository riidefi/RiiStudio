#pragma once

#include "SysDolphin/allincludes.hpp"

namespace libcube { namespace pikmin1 {

struct DCAAnimJoint
{
	u32 index = 0;
	u32 parent = 0;

	std::array<u32, 3> sx_param, sy_param;
	std::array<u32, 3> rx_param, ry_param;
	std::array<u32, 3> tx_param, ty_param;

	DCAAnimJoint() = default;
	~DCAAnimJoint() = default;
};

struct DCA
{
	constexpr static u32 bundleType = 2;
	constexpr static const char name[] = "Demo Cutscene Animation (DCA)";
	u32 m_numJoints = 0;
	u32 m_numFrames = 0;

	DataChunk m_scaling;
	DataChunk m_rotation;
	DataChunk m_translation;
	std::vector<DCAAnimJoint> m_joints;


	DCA() = default;
	~DCA() = default;


	static void onRead(oishii::BinaryReader& bReader, DCA& context);
	static inline void writeType(oishii::Writer& bWriter)
	{
		bWriter.write<u32>(DCA::bundleType);
	}
};

}

}
