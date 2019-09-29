#pragma once

#include "SysDolphin/allincludes.hpp"

namespace libcube { namespace pikmin1 {

struct DCAAnimJoint
{
	u32 index = 0;
	u32 parent = 0;

	std::pair<u32, u32> sx_param[3], sy_param[3];
	std::pair<u32, u32> rx_param[3], ry_param[3];
	std::pair<u32, u32> tx_param[3], ty_param[3];

	DCAAnimJoint() = default;
	~DCAAnimJoint() = default;
};

struct DCA
{
	constexpr static int bundleType = 2;
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
