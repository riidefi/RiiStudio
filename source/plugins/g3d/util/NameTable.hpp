#pragma once

#include <core/common.h>

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>

namespace riistudio::g3d {

using NameTable = librii::g3d::NameTable;

inline void ApplyGlobalRelocToOishii(NameTable& table, oishii::Writer& writer,
                                     librii::g3d::NameReloc reloc) {
  table.reserve(reloc.name,                        // name
                reloc.offset_of_delta_reference,   // structPos
                writer,                            // writeStream
                reloc.offset_of_pointer_in_struct, // writePos
                reloc.non_volatile                 // nonvolatile
  );
}

inline std::string_view readName(oishii::BinaryReader& reader,
                                 std::size_t start) {
  const auto ofs = reader.read<s32>();

  if (ofs && ofs + start < reader.endpos() && ofs + start > 0) {
    return reinterpret_cast<const char*>(reader.getStreamStart()) + start + ofs;
  } else {
    return "";
  }
}

} // namespace riistudio::g3d
