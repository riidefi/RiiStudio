#pragma once

#include <core/kpi/Plugins.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <plugins/mk/KMP/Map.hpp>
#include <string>

namespace riistudio::mk {

class KMP {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {
    return file.ends_with("kmp") ? typeid(CourseMap).name() : "";
  }
  void read(kpi::IOTransaction& transaction) const;
  bool canWrite(kpi::INode& node) const {
    return dynamic_cast<CourseMap*>(&node) != nullptr;
  }
  void write(kpi::INode& node, oishii::Writer& writer) const;
};
} // namespace riistudio::mk
