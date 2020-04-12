#pragma once

#include <core/common.h>

#include "GPUAddressSpace.hpp"
#include "GPUCommand.hpp"

#include <oishii/reader/binary_reader.hxx>

#include <plugins/gc/gx/VertexTypes.hpp>

namespace libcube::gpu {

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
  virtual void onCommandBP(const QBPCommand& token) {}
  virtual void onCommandCP(const QCPCommand& token) {}
  virtual void onCommandXF(const QXFCommand& token) {}
  virtual void onCommandDraw(oishii::BinaryReader& reader,
                             libcube::gx::PrimitiveType type, u16 nverts) {}

  virtual void onStreamBegin() {}
  virtual void onStreamEnd() {}
};

void RunDisplayList(oishii::BinaryReader& reader, QDisplayListHandler& handler,
                    u32 dlSize);

} // namespace libcube::gpu
