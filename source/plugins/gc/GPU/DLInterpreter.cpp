#include "DLInterpreter.hpp"

namespace libcube::gpu {

void RunDisplayList(oishii::BinaryReader& reader, QDisplayListHandler& handler,
                    u32 dlSize) {
  const auto start = reader.tell();

  handler.onStreamBegin();

  while (reader.tell() < start + (int)dlSize) {
    CommandType tag = static_cast<CommandType>(reader.readUnaligned<u8>());

    switch (tag) {
    case CommandType::BP: {
      QBPCommand cmd;
      u32 rv = reader.readUnaligned<u32>();
      cmd.reg = static_cast<BPAddress>((rv & 0xff000000) >> 24);
      cmd.val = (rv & 0x00ffffff);
      handler.onCommandBP(cmd);
      break;
    }
    case CommandType::NOP:
      break;
    case CommandType::XF: {
      // TODO: Verify
      QXFCommand cmd;
      const auto nCmd = reader.readUnaligned<u16>();
      const auto reg = reader.readUnaligned<u16>();
      cmd.reg = reg;
      cmd.val = reader.readUnaligned<u32>();
      cmd.vals.resize(nCmd + 1);
      cmd.vals[0] = cmd.val;
      for (u16 i = 0; i < nCmd; ++i) {
        cmd.vals[i] = reader.readUnaligned<u32>();
      }
      handler.onCommandXF(cmd);
      break;
    }
    case CommandType::CP: {
      QCPCommand cmd;
      cmd.reg = reader.readUnaligned<u8>();
      cmd.val = reader.readUnaligned<u32>();

      handler.onCommandCP(cmd);
      break;
    }
    default:
      if (static_cast<u32>(tag) & 0x80) {
        handler.onCommandDraw(
            reader,
            libcube::gx::DecodeDrawPrimitiveCommand(static_cast<u32>(tag)),
            reader.readUnaligned<u16>());
      }
      // TODO
      break;
    }
  }

  handler.onStreamEnd();
}

} // namespace libcube::gpu
