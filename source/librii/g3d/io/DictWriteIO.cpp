#include "DictWriteIO.hpp"
#include "CommonIO.hpp"
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

namespace librii::g3d {

using namespace bad;

// Based on wszst code on the wiki page
static u16 get_highest_bit(u8 val) {
  u16 i = 7;
  for (; i > 0 && !(val & 0x80); i--, val <<= 1)
    ;
  return i;
}
static u16 calc_brres_id(const char* object_name, u32 object_len,
                         const char* subject_name, u32 subject_len) {
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
  DictionaryNode& entry = mNodes[entry_idx];
  entry.mId = calc_brres_id(0, 0, entry.mName.c_str(), entry.mName.size());
  entry.mIdxPrev = entry.mIdxNext = entry_idx;

  DictionaryNode& prev = mNodes[0];
  u32 current_idx = prev.mIdxPrev;
  DictionaryNode& current = mNodes[current_idx];

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
  DictionaryNode& root = mNodes[0];
  root.mId = 0xffff;
  root.mIdxPrev = root.mIdxNext = 0;

  // Calculate entries
  for (int i = 0; i < mNodes.size(); i++)
    calcNode(i);
}

void Dictionary::read(oishii::BinaryReader& reader) {
  mNodes.clear();
  const auto grpStart = reader.tell();

  // Note: Entry count does not include the root.
  const auto [totalSize, nEntry] = reader.readX<u32, 2>();

  for (u32 i = 0; i <= nEntry; i++)
    mNodes.emplace_back(reader, grpStart);

  assert(totalSize == reader.tell() - grpStart);
}
void Dictionary::write(oishii::Writer& writer, NameTable& table) {
  calcNodes();

  const auto grpStart = writer.tell();

  const auto ofsTotalSize = grpStart;
  writer.write<u32>(0);

  writer.write<u32>(static_cast<u32>(mNodes.size() - 1)); // nEntry

  for (auto& node : mNodes)
    node.write(writer, grpStart, table);

  const auto totalSize = writer.tell() - ofsTotalSize;

  {
    oishii::Jump<oishii::Whence::Set, oishii::Writer> g(writer, ofsTotalSize);

    writer.write<u32>(totalSize);
  }
}

void DictionaryNode::read(oishii::BinaryReader& reader,
                          u32 groupStartPosition) {
  mId = reader.read<u16>();
  mFlag = reader.read<u16>();
  mIdxPrev = reader.read<u16>();
  mIdxNext = reader.read<u16>();
  mName = readName(reader, groupStartPosition);

  mDataDestination = groupStartPosition + reader.read<s32>();
  if (mDataDestination == groupStartPosition)
    mDataDestination = 0;
}
void DictionaryNode::write(oishii::Writer& writer, u32 groupStartPosition,
                           NameTable& table) const {
  writer.write<u16>(mId);
  writer.write<u16>(mFlag);
  writer.write<u16>(mIdxPrev);
  writer.write<u16>(mIdxNext);
  writeNameForward(table, writer, groupStartPosition, mName);
  writer.write<s32>(mDataDestination ? mDataDestination - groupStartPosition
                                     : 0);
}
void QDictionaryNode::read(oishii::BinaryReader& reader,
                           u32 groupStartPosition) {
  mId = reader.read<u16>();
  mFlag = reader.read<u16>();
  mIdxPrev = reader.read<u16>();
  mIdxNext = reader.read<u16>();
  mName = readName(reader, groupStartPosition);

  mDataDestination = groupStartPosition + reader.read<s32>();
  if (mDataDestination == groupStartPosition)
    mDataDestination = 0;
}
void QDictionaryNode::write(oishii::Writer& writer, u32 groupStartPosition,
                            NameTable& table) const {
  writer.write<u16>(mId);
  writer.write<u16>(mFlag);
  writer.write<u16>(mIdxPrev);
  writer.write<u16>(mIdxNext);
  writeNameForward(table, writer, groupStartPosition, mName);
  writer.write<s32>(mDataDestination ? mDataDestination - groupStartPosition
                                     : 0);
}

QDictionaryNode& QDictionary::cGet() {
  while (cursor >= mNodes.size()) {
    printf("Inserting dummy node. This will change size of dictionary that may "
           "be reserved!\n");
    mNodes.push_back(QDictionaryNode());
  }

  return mNodes[cursor++];
}

void QDictionary::calcNode(u32 entry_idx) {
  // setup 'entry' : item to insert
  QDictionaryNode* entry = &mNodes[entry_idx];
  entry->mId = calc_brres_id(0, 0, entry->mName.c_str(), entry->mName.size());
  entry->mIdxPrev = entry->mIdxNext = entry_idx;

  // setup 'prev' : the previuos 'current' item
  QDictionaryNode* prev = &mNodes[0];

  // setup 'current' : the current item whule walking through the tree
  u32 current_idx = prev->mIdxPrev;
  QDictionaryNode* current = &mNodes[current_idx];

  // last direction
  bool is_right = false;

  while (entry->mId <= current->mId && current->mId < prev->mId) {
    if (entry->mId == current->mId) {
      // calculate a new entry id
      entry->mId = calc_brres_id(current->mName.c_str(), current->mName.size(),
                                 entry->mName.c_str(), entry->mName.size());
      if (current->calcIdBit(entry->mId)) {
        entry->mIdxPrev = entry_idx;
        entry->mIdxNext = current_idx;
      } else {
        entry->mIdxPrev = current_idx;
        entry->mIdxNext = entry_idx;
      }
    }

    prev = current;
    is_right = entry->calcIdBit(current->mId);
    current_idx = is_right ? current->mIdxNext : current->mIdxPrev;
    current = &mNodes[current_idx];
  }

  if (current->mName.size() == entry->mName.size() &&
      current->calcIdBit(entry->mId))
    entry->mIdxNext = current_idx;
  else
    entry->mIdxPrev = current_idx;

  if (is_right)
    prev->mIdxNext = entry_idx;
  else
    prev->mIdxPrev = entry_idx;
}
void QDictionary::calcNodes() {
  assert(!mNodes.empty());

  // Create root
  QDictionaryNode& root = getRoot();
  root.mId = 0xffff;
  root.mIdxPrev = root.mIdxNext = 0;

  // Calculate entries
  for (int i = 0; i < mNodes.size(); i++)
    calcNode(i);
}

void QDictionary::read(oishii::BinaryReader& reader) {
  mNodes.clear();
  u32 grpStart = reader.tell();
  MAYBE_UNUSED u32 totalSize = reader.read<u32>();
  u32 nEntry = reader.read<u32>(); // Not including root

  for (u32 i = 0; i <= nEntry; i++)
    mNodes.push_back(QDictionaryNode(reader, grpStart));

  assert(totalSize == reader.tell() - grpStart);
}

void QDictionary::write(oishii::Writer& writer, NameTable& names) {
  calcNodes();

  u32 grpStart = writer.tell();

  auto ofsTotalSize = writer.tell();
  writer.write<u32>(0, /* checkmatch */ false);
  writer.write<u32>(mNodes.size() - 1); // nEntry

  for (auto& node : mNodes)
    node.write(writer, grpStart, names);

  writer.writeAt<u32>(writer.tell() - ofsTotalSize, ofsTotalSize);
}

} // namespace librii::g3d
