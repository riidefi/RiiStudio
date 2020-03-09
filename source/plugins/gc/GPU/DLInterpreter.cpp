#include "DLInterpreter.hpp"

namespace libcube::gpu {

void RunDisplayList(oishii::BinaryReader& reader, QDisplayListHandler& handler, u32 dlSize) {
	const auto start = reader.tell();

	handler.onStreamBegin();

	while (reader.tell() < start + (int)dlSize)
	{
		CommandType tag = static_cast<CommandType>(reader.readUnaligned<u8>());

		switch (tag)
		{
		case CommandType::BP:
		{
			QBPCommand cmd;
			u32 rv = reader.readUnaligned<u32>();
			cmd.reg = static_cast<BPAddress>((rv & 0xff000000) >> 24);
			cmd.val = (rv & 0x00ffffff);
			handler.onCommandBP(cmd);
			break;
		}
		case CommandType::NOP:
			break;
		case CommandType::XF:
		{
			// TODO
			break;
		}
		case CommandType::CP:
		{
			QCPCommand cmd;
			cmd.reg = reader.readUnaligned<u8>();
			cmd.val = reader.readUnaligned<u32>();

			handler.onCommandCP(cmd);
			break;
		}
		default:
			// TODO
			break;
		}
	}

	handler.onStreamEnd();
}


} // namespace libcube::gpu
