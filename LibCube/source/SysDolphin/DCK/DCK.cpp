#include "DCK.hpp"

namespace libcube {
namespace pikmin1 {

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
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.sx_param[i]));
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.sy_param[i]));
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.sz_param[i]));
		}
		for (u32 i = 0; i < 3; i++)
		{
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.rx_param[i]));
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.ry_param[i]));
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.rz_param[i]));
		}
		for (u32 i = 0; i < 3; i++)
		{
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.tx_param[i]));
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.ty_param[i]));
			std::apply([&bReader](auto && ... args) { ((args = bReader.read<u32>()), ...); }, std::move(joint.tz_param[i]));
		}

	}
}

}

}
