#include "Dictionary.hpp"

#include <oishii/reader/binary_reader.hxx>
#include <oishii/v2/writer/binary_writer.hxx>

#include <plugins/g3d/util/NameTable.hpp>

namespace riistudio::g3d {

// Based on wszst code on the wiki page
static u16 get_highest_bit(u8 val) {
  u16 i = 7;
  for (; i > 0 && !(val & 0x80); i--, val <<= 1)
    ;
  return i;
}
static u16 calc_brres_id(const char *object_name, u32 object_len,
                         const char *subject_name, u32 subject_len) {
  if (object_len < subject_len)
    return (subject_len - 1) << 3 |
           get_highest_bit(subject_name[subject_len - 1]);

  while (subject_len-- > 0) {
    const u8 ch = object_name[subject_len] ^ subject_name[subject_len];
    if (ch)
      return subject_len << 3 | get_highest_bit(ch);
  }

  return ~(u16)0;
}

void Dictionary::calcNode(u32 entry_idx) {
  DictionaryNode &entry = mNodes[entry_idx];
  entry.mId = calc_brres_id(0, 0, entry.mName.c_str(), entry.mName.size());
  entry.mIdxPrev = entry.mIdxNext = entry_idx;

  DictionaryNode &prev = mNodes[0];
  u32 current_idx = prev.mIdxPrev;
  DictionaryNode &current = mNodes[current_idx];

  bool is_right = false;

  while (entry.mId <= current.mId && current.mId < prev.mId) {
    if (entry.mId == current.mId) {
      entry.mId = calc_brres_id(current.mName.c_str(), current.mName.size(),
                                entry.mName.c_str(), entry.mName.size());
      if (current.calcId(entry.mId)) {
        entry.mIdxPrev = entry_idx;
        entry.mIdxNext = current_idx;
      } else {
        entry.mIdxPrev = current_idx;
        entry.mIdxNext = entry_idx;
      }
    }

    prev = current;
    is_right = entry.calcId(current.mId);
    current_idx = is_right ? current.mIdxNext : current.mIdxPrev;
    current = mNodes[current_idx];
  }

  if (current.mName.size() == entry.mName.size() && current.calcId(entry.mId))
    entry.mIdxNext = current_idx;
  else
    entry.mIdxPrev = current_idx;

  if (is_right)
    prev.mIdxNext = entry_idx;
  else
    prev.mIdxPrev = entry_idx;
}
void Dictionary::calcNodes() {
  assert(!mNodes.empty());

  // Create root
  DictionaryNode &root = mNodes[0];
  root.mId = 0xffff;
  root.mIdxPrev = root.mIdxNext = 0;

  // Calculate entries
  for (int i = 0; i < mNodes.size(); i++)
    calcNode(i);
}

void Dictionary::read(oishii::BinaryReader &reader) {
  mNodes.clear();
  const auto grpStart = reader.tell();

  // Note: Entry count does not include the root.
  const auto [totalSize, nEntry] = reader.readX<u32, 2>();

  for (u32 i = 0; i <= nEntry; i++)
    mNodes.emplace_back(reader, grpStart);

  assert(totalSize == reader.tell() - grpStart);
}
void Dictionary::write(oishii::v2::Writer &writer) {
  calcNodes();

  const auto grpStart = writer.tell();

  const auto ofsTotalSize = grpStart;
  writer.write<u32>(0);

  writer.write<u32>(static_cast<u32>(mNodes.size() - 1)); // nEntry

  //	for (auto& node : mNodes)
  //		node.write(writer, grpStart);

  const auto totalSize = writer.tell() - ofsTotalSize;

  {
    oishii::Jump g(writer, ofsTotalSize);

    writer.write<u32>(totalSize);
  }
}

void DictionaryNode::read(oishii::BinaryReader &reader,
                          u32 groupStartPosition) {
  mId = reader.read<u16>();
  mFlag = reader.read<u16>();
  mIdxPrev = reader.read<u16>();
  mIdxNext = reader.read<u16>();
  mName = readName(reader, groupStartPosition);

  mDataDestination = groupStartPosition + reader.read<s32>();
  if (mDataDestination == groupStartPosition)
    mDataDestination = 0;
  // On write, write dataDest - grpStart
}

} // namespace riistudio::g3d
