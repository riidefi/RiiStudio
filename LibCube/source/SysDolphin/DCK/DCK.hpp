#pragma once

#include "SysDolphin/allincludes.hpp"
#include <tuple>

namespace libcube { namespace pikmin1 {

struct DCKAnimJoint
{
	u32 index = 0;
	u32 parent = 0;

	std::array<std::tuple<f32, f32, f32>, 3> sx_param, sy_param, sz_param;
	std::array<std::tuple<f32, f32, f32>, 3> rx_param, ry_param, rz_param;
	std::array<std::tuple<f32, f32, f32>, 3> tx_param, ty_param, tz_param;

	DCKAnimJoint() = default;
	~DCKAnimJoint() = default;
};

struct DCK
{
	constexpr static u32 bundleType = 3;
	constexpr static const char name[] = "Demo Cutscene Keyframed (DCK)";
	u32 m_numJoints = 0;
	u32 m_numFrames = 0;

	DataChunk m_scaling;
	DataChunk m_rotation;
	DataChunk m_translation;
	std::vector<DCKAnimJoint> m_joints;


	DCK() = default;
	~DCK() = default;


	static void onRead(oishii::BinaryReader& bReader, DCK& context);
	static inline void writeType(oishii::Writer& bWriter)
	{
		bWriter.write<u32>(DCK::bundleType);
	}
};

}

}
