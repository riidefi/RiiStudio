#pragma once

#include <core/kpi/Plugins.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <plugins/mk/BFG/Fog.hpp>

namespace riistudio::mk {

class BFG {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {
    return file.ends_with("bfg") ? typeid(BinaryFog).name() : "";
  }

  void read(kpi::IOTransaction& transaction) const;
  bool canWrite(kpi::INode& node) const {
    return dynamic_cast<BinaryFog*>(&node) != nullptr;
  }
  Result<void> write(kpi::INode& node, oishii::Writer& writer) const;
};

} // namespace riistudio::mk
