// This class is a relic of the old tool.

#include "NameTableIO.hpp"
IMPORT_STD;

namespace librii::g3d {

template <typename T> static void writeAt(T& stream, u32 pos, s32 val) {
  auto back = stream.tell();
  stream.seekSet(pos);
  stream.template write<s32>(val);
  stream.seekSet(back);
}

void writeNameForward(NameTable& table, oishii::Writer& writer,
                      int streamOfsStart, const std::string& name,
                      bool nonvol) {
  if (name.empty()) {
    writer.write<u32>(0);
    return;
  }

  writer.write<u32>(
      table.reserve(name, streamOfsStart, writer, writer.tell(), nonvol),
      false);
}

NameTable::Handle NameTable::reserve(const std::string& name, u32 structPos,
                                     oishii::Writer& writeStream, u32 writePos,
                                     bool nonvolatile) {
  assert(structPos < 0xffffff);
  const Handle id = mCounter++;
  mEntries.emplace_back(
      NameTableEntry{name, structPos, &writeStream, writePos, nonvolatile, id});
  return id;
}

std::size_t NameTable::resolveName(Handle id, u32 structStartOfs, u32 poolOfs) {
  assert(mMapping.contains(id) && mMapping[id] < 0);
  const std::size_t poolentry =
      static_cast<std::size_t>(-mMapping[id] + poolOfs);
  // printf("Name %u: From %u to %u+%u, delta: %u\n", id, structStartOfs,
  //        poolOfs, (u32)-mMapping[id], (u32)poolentry - structStartOfs);
  return poolentry - structStartOfs;
}

void NameTable::poolNames(bool UseNMethod) {
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

void NameTable::resolve(u32 pool) {
  for (const auto& entry : mEntries) {
    writeAt(*entry.writeStream, entry.writePos,
            resolveName(entry.id, entry.structPos, pool));
  }
  mEntries.clear();
}

} // namespace librii::g3d
