#pragma once

#include "GPUAddressSpace.hpp"
#include "GPUCommand.hpp"
#include <core/common.h>
#include <librii/gx.h>
#include <oishii/reader/binary_reader.hxx>

namespace librii::gpu {

struct QBPCommand {
  BPAddress reg;
  u32 val;
};

struct QXFCommand {
  u16 reg;
  u32 val; // first val

  std::vector<u32> vals; // first val repeated
};
struct QCPCommand {
  u8 reg;
  u32 val;
};

class QDisplayListHandler {
public:
  virtual ~QDisplayListHandler() {}
  virtual Result<void> onCommandBP(const QBPCommand& token) { return {}; }
  virtual Result<void> onCommandCP(const QCPCommand& token) { return {}; }
  virtual Result<void> onCommandXF(const QXFCommand& token) { return {}; }
  virtual Result<void> onCommandDraw(oishii::BinaryReader& reader,
                                     librii::gx::PrimitiveType type, u16 nverts,
                                     u32 stream_end) {
    return {};
  }
  virtual Result<void> onCommandIndexedLoad(u32 cmd, u32 index, u16 address,
                                            u8 size) {
    return {};
  }

  virtual Result<void> onStreamBegin() { return {}; }
  virtual Result<void> onStreamEnd() { return {}; }
};

Result<void> RunDisplayList(oishii::BinaryReader& reader,
                            QDisplayListHandler& handler, u32 dlSize);

} // namespace librii::gpu
