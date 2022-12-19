#include "BFG.hpp"
#include <core/util/glm_io.hpp>
#include <functional>
#include <llvm/ADT/StringMap.h>
#include <numeric>
#include <oishii/writer/binary_writer.hxx>
#include <sstream>

namespace riistudio::mk {

void BFG::read(kpi::IOTransaction& transaction) const {
  BinaryFog& fog = static_cast<BinaryFog&>(transaction.node);
  oishii::BinaryReader reader(std::move(transaction.data));
  kpi::IOContext ctx("bfg", transaction);

  auto entries = fog.getFogEntries();
  entries.resize(4); // Always 4 sections?
  for (auto& entry : entries) {
    entry.mType = static_cast<librii::gx::FogType>(reader.read<u32>());
    entry.mStartZ = reader.read<f32>();
    entry.mEndZ = reader.read<f32>();
    entry.mColor.r = reader.read<u8>();
    entry.mColor.g = reader.read<u8>();
    entry.mColor.b = reader.read<u8>();
    entry.mColor.a = reader.read<u8>();
    entry.mEnabled = reader.read<u16>();
    entry.mCenter = reader.read<u16>();
    entry.mFadeSpeed = reader.read<f32>();
    entry.mUnk18 = reader.read<u16>();
    entry.mUnk1A = reader.read<u16>();
  }
}

void BFG::write(kpi::INode& node, oishii::Writer& writer) const {
  const BinaryFog& fog = *dynamic_cast<BinaryFog*>(&node);
  writer.setEndian(std::endian::big);

  auto entries = fog.getFogEntries();
  for (auto& entry : entries) {
    writer.write<u32>(static_cast<u32>(entry.mType));
    writer.write<f32>(entry.mStartZ);
    writer.write<f32>(entry.mEndZ);
    writer.write<u8>(entry.mColor.r);
    writer.write<u8>(entry.mColor.g);
    writer.write<u8>(entry.mColor.b);
    writer.write<u8>(entry.mColor.a);
    writer.write<u16>(entry.mEnabled);
    writer.write<u16>(entry.mCenter);
    writer.write<f32>(entry.mFadeSpeed);
    writer.write<u16>(entry.mUnk18);
    writer.write<u16>(entry.mUnk1A);
  }
}

} // namespace riistudio::mk
