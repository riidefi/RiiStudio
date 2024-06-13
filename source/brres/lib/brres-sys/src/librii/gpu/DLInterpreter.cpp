#include "DLInterpreter.hpp"

#include <rsl/SafeReader.hpp>

namespace librii::gpu {

Result<void> RunDisplayList(oishii::BinaryReader& unsafeReader,
                            QDisplayListHandler& handler, u32 dlSize) {
  rsl::SafeReader reader(unsafeReader);
  const auto start = reader.tell();

  TRY(handler.onStreamBegin());

  while (reader.tell() < start + (int)dlSize) {
    CommandType tag = static_cast<CommandType>(TRY(reader.U8NoAlign()));

    switch (tag) {
    case CommandType::BP: {
      QBPCommand cmd;
      u32 rv = TRY(reader.U32NoAlign());
      cmd.reg = static_cast<BPAddress>((rv & 0xff000000) >> 24);
      cmd.val = (rv & 0x00ffffff);
      TRY(handler.onCommandBP(cmd));
      break;
    }
    case CommandType::NOP:
      break;
    case CommandType::XF: {
      // TODO: Verify
      QXFCommand cmd;
      const auto nCmd = TRY(reader.U16NoAlign());
      const auto reg = TRY(reader.U16NoAlign());
      cmd.reg = reg;
      cmd.val = TRY(reader.U32NoAlign());
      cmd.vals.resize(nCmd + 1);
      cmd.vals[0] = cmd.val;
      for (u16 i = 0; i < nCmd; ++i) {
        cmd.vals[i] = TRY(reader.U32NoAlign());
      }
      TRY(handler.onCommandXF(cmd));
      break;
    }
    case CommandType::CP: {
      QCPCommand cmd;
      cmd.reg = TRY(reader.U8NoAlign());
      cmd.val = TRY(reader.U32NoAlign());

      TRY(handler.onCommandCP(cmd));
      break;
    }
    case CommandType::LOAD_INDX_A: // Position matrices - Start at 0, len=12
                                   // (3x4)
    case CommandType::LOAD_INDX_B: // Normal matrices - Start at 1024, len=9
                                   // (3x3)
    case CommandType::LOAD_INDX_C: // Postmatrices - ??
    case CommandType::LOAD_INDX_D: // Lights - ??
    {
      u32 val = TRY(reader.U32NoAlign());
      u16 index = val >> 16;
      u16 addr = val & 0x0FFF;
      u8 len = ((val >> 12) & 0xF) + 1;
      TRY(handler.onCommandIndexedLoad(static_cast<u8>(tag), index, addr, len));
      break;
    }
    default:
      if (static_cast<u32>(tag) & 0x80) {
        auto prim =
            librii::gx::DecodeDrawPrimitiveCommand(static_cast<u32>(tag));
        auto verts = TRY(reader.U16NoAlign());
        TRY(handler.onCommandDraw(unsafeReader, prim, verts, start + dlSize));
      } else {
        return RSL_UNEXPECTED(std::format("Unrecognized command {} in stream.",
                                           static_cast<u32>(tag)));
      }
      break;
    }
  }
  TRY(handler.onStreamEnd());
  return {};
}

} // namespace librii::gpu
