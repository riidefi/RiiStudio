#pragma once

#include <core/common.h>

#include <map>
#include <string>
#include <vector>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/v2/writer/binary_writer.hxx>

namespace riistudio::g3d {

class NameTable {
public:
  using Handle = s32;

  //! @brief Add a name to the table. This name will then be written.
  //!
  //! @param[in] name         The name to write.
  //! @param[in] structPos    Start of the structure in the same space as the
  //! name table will be written; used later to compute offset.
  //! @param[in] writeStream  Stream which will be used to write this entry.
  //! @param[in] writePos     Where in the write stream to put the resolved
  //! offset. (The runtime string offset)
  //! @param[in] nonvolatile  If the name must be preserved: "3DModels(NW4R)",
  //! "DrawOpa", etc.
  //!
  //! @return ID of this name.
  //!
  Handle reserve(const std::string& name, u32 structPos,
                 oishii::v2::Writer& writeStream, u32 writePos,
                 bool nonvolatile = false) {
    const Handle id = mCounter++;
    mEntries.push_back(
        NameTableEntry{name, structPos, writeStream, writePos, nonvolatile});
    return id;
  }

  inline std::size_t resolveName(Handle id, u32 structStartOfs, u32 poolOfs) {
    const std::size_t poolentry =
        static_cast<std::size_t>(mMapping[id] + poolOfs);
    return poolentry - structStartOfs;
  }

  // TODO:
  // poolnames
  // resolve

private:
  struct NameTableEntry {
    std::string name;
    u32 structPos;
    oishii::v2::Writer& writeStream;
    u32 writePos;
    bool nonvolatile;
  };
  std::size_t mCounter = 0; //!< Necessary as the vector may shrink
  std::vector<NameTableEntry> mEntries;

  // Pool
  std::map<Handle, std::size_t> mMapping;
  std::vector<u8> mPool;
};

inline void writeNameForward(NameTable& table, oishii::v2::Writer& writer,
                             int streamOfsStart, const std::string& name,
                             bool nonvol = false) {
  writer.write<u32>(name.empty() ? 0
                                 : table.reserve(name, streamOfsStart, writer,
                                                 writer.tell(), nonvol));
}
inline std::string readName(oishii::BinaryReader& reader, std::size_t start) {
  const auto ofs = reader.read<s32>();

  return (ofs && ofs + start < reader.endpos() && ofs + start > 0)
             ? std::string(
                   reinterpret_cast<const char*>(reader.getStreamStart()) +
                   start + ofs)
             : "";
}

} // namespace riistudio::g3d
