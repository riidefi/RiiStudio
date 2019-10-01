#include "DCK.hpp"

#include <LibRiiEditor/common.hpp>

namespace libcube { namespace pikmin1 {

void DCK::onRead(oishii::BinaryReader& bReader, DCK& context)
{
	context.m_numJoints = bReader.read<u32>();
	context.m_numFrames = bReader.read<u32>();
	context.m_scaling << bReader;
	context.m_rotation << bReader;
	context.m_translation << bReader;

	context.m_joints.resize(context.m_numJoints);
	for (auto& joint : context.m_joints)
	{
		joint.index = bReader.read<u32>();
		joint.parent = bReader.read<u32>();
		if (joint.parent == -1)
			DebugReport("Joint %d has no parent!", joint.index);

		for (u32 i = 0; i < 3; i++)
		{
		// FIXME: Apply fixed point shift and fill in vector3
			joint.sx_param[i] << bReader;
			joint.sy_param[i] << bReader;
			joint.sz_param[i] << bReader;
		}
		for (u32 i = 0; i < 3; i++)
		{
			joint.rx_param[i] << bReader;
			joint.ry_param[i] << bReader;
			joint.rz_param[i] << bReader;
		}
		for (u32 i = 0; i < 3; i++)
		{
			joint.tx_param[i] << bReader;
			joint.ty_param[i] << bReader;
			joint.tz_param[i] << bReader;
		}

	}
}

} }
