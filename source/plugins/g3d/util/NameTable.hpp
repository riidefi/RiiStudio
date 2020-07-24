#pragma once

#include <core/common.h>

#include <map>
#include <string>
#include <vector>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

namespace riistudio::g3d {

// This class is a relic of the old tool.
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
                 oishii::Writer& writeStream, u32 writePos,
                 bool nonvolatile = false) {
    assert(structPos < 0xffffff);
    const Handle id = mCounter++;
    mEntries.emplace_back(NameTableEntry{name, structPos, &writeStream,
                                         writePos, nonvolatile, id});
    return id;
  }

  inline std::size_t resolveName(Handle id, u32 structStartOfs, u32 poolOfs) {
    assert(mMapping.contains(id) && mMapping[id] < 0);
    const std::size_t poolentry =
        static_cast<std::size_t>(-mMapping[id] + poolOfs);
    // printf("Name %u: From %u to %u+%u, delta: %u\n", id, structStartOfs,
    //        poolOfs, (u32)-mMapping[id], (u32)poolentry - structStartOfs);
    return poolentry - structStartOfs;
  }

  //! @brief Construct the string pool.
  //!
  void poolNames(bool UseNMethod = true) {
    // Clear the pool
    mMapping.clear();
    mPool.clear();

    // Pool names without duplicates
    // TODO: this can be more efficient, no need for duplicate "pool" vector
    int size = 0; // size of all names + terminating zeroes
    std::vector<std::pair<std::string, bool>> pool; // Name, NonVolatile

    std::sort(mEntries.begin(), mEntries.end(),
              [](const auto& s, const auto& s2) { return s.name < s2.name; });

    for (auto& it : mEntries) {
      auto pastIt = std::find_if(pool.begin(), pool.end(),
                                 [it](const std::pair<std::string, bool>& s) {
                                   return s.first == it.name;
                                 });

      if (pastIt == pool.end()) {
        mMapping[it.id] = pool.size();
        pool.push_back({it.name, it.nonvolatile});
        size += it.name.size() + 5;
      } else { // Duplicate
        // std::cout << "Duplicate name " << it.name
        //           << " (past Idx: " << std::to_string(pastIt - pool.begin())
        //           << ")!\n";
        mMapping[it.id] = pastIt - pool.begin();
      }
    }

    // Construct binary pool
    mPool.reserve(size);
    int i = 0;
    for (const auto& [name, nonvolatile] : pool) {
      if (UseNMethod) {
        u32 sz = name.size();
        mPool.push_back((sz & 0xff000000) >> 24);
        mPool.push_back((sz & 0x00ff0000) >> 16);
        mPool.push_back((sz & 0x0000ff00) >> 8);
        mPool.push_back((sz & 0x000000ff) >> 0);
      }
      // Resolve indices to offsets
      for (auto& mapIt : mMapping)
        if (mapIt.second == i)
          mapIt.second = -mPool.size();
      // Push the string
      for (char c : (nonvolatile ? name : name))
        mPool.push_back(c);
      mPool.push_back(0);

      if (UseNMethod) {
        while (mPool.size() % 4)
          mPool.push_back(0);
      }

      ++i;
    }

    if (mPool.size() != size) {
      std::cout << "[Warning] Expected string pool of size "
                << std::to_string(size) << ". Actual size is "
                << std::to_string(mPool.size()) << "!\n";
    }
  }
  template <typename T> static void writeAt(T& stream, u32 pos, s32 val) {
    auto back = stream.tell();
    stream.seekSet(pos);
    stream.template write<s32>(val);
    stream.seekSet(back);
  }
  //! @brief Resolve all reservations.
  //!
  //! @details Reservation pointers will be set to offsets to the string table.
  //!
  //! @param[in] Offset of pool in output stream
  //!
  void resolve(u32 pool) {
    for (const auto& entry : mEntries) {
      writeAt(*entry.writeStream, entry.writePos,
              resolveName(entry.id, entry.structPos, pool));
    }
    mEntries.clear();
  }

private:
  struct NameTableEntry {
    std::string name;
    u32 structPos;
    oishii::Writer* writeStream;
    u32 writePos;
    bool nonvolatile;
    Handle id;
  };
  std::size_t mCounter = 0; //!< Necessary as the vector may shrink
  std::vector<NameTableEntry> mEntries;

  // Pool
  std::map<Handle, s32> mMapping;

public:
  std::vector<u8> mPool;
};

inline void writeNameForward(NameTable& table, oishii::Writer& writer,
                             int streamOfsStart, const std::string& name,
                             bool nonvol = false) {
  if (name.empty()) {
    writer.write<u32>(0);
    return;
  }

  writer.write<u32>(
      table.reserve(name, streamOfsStart, writer, writer.tell(), nonvol));
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
