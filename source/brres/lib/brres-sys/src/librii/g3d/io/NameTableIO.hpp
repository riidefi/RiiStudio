#pragma once

#include <core/util/oishii.hpp>
#include <map>
#include <memory>
#include <string>

namespace librii::g3d {

class NameTable {
public:
  using Handle = s32;

  Handle reserve(const std::string& name, u32 structPos,
                 oishii::Writer& writeStream, u32 writePos,
                 bool nonvolatile = false);

  std::size_t resolveName(Handle id, u32 structStartOfs, u32 poolOfs);

  void poolNames(bool UseNMethod = true);

  void resolve(u32 pool);

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

void writeNameForward(NameTable& table, oishii::Writer& writer,
                      int streamOfsStart, const std::string& name,
                      bool nonvol = false);

} // namespace librii::g3d
