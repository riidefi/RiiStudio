#include "DictWriteIO.hpp"
#include "CommonIO.hpp"
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

namespace librii::g3d {

namespace bad {
// Writing is broken, don't know why, don't use it
struct DictionaryNode {
  u16 mId = 0;
  u16 mFlag = 0;
  u16 mIdxPrev = 0;
  u16 mIdxNext = 0;

  void resetInternal() { mId = mFlag = mIdxPrev = mIdxNext = 0; }
  std::string mName;

  s32 mDataDestination = 0;

  inline bool calcId(u16 id) {
    const u32 c = id >> 3;

    return c < mName.size() && ((mName[c] >> (id & 7)) & 1);
  }

  Result<void> read(rsl::SafeReader& reader, u32 start);
  void write(oishii::Writer& writer, u32 groupStartPosition,
             NameTable& table) const;

  DictionaryNode() = default;
  DictionaryNode(const std::string& name) : mName(name) {}
};

struct Dictionary {
  std::vector<DictionaryNode> mNodes;

  void calcNode(u32 id);
  void calcNodes();

  Result<void> read(rsl::SafeReader& reader);
  void write(oishii::Writer& writer, NameTable& table);

  Dictionary() : mNodes(1){};
  Dictionary(std::size_t num_nodes) : mNodes(num_nodes) {}
  std::size_t computeSize() const { return 8 + 16 * mNodes.size(); }
  void emplace(const std::string& name) { mNodes.emplace_back(name); }
};

} // namespace bad

// It's bad, but it works. Don't use it.
class QDictionaryNode {
  friend class QDictionary;

  // Internal values--necessary for the runtime library.
  u16 mId, mFlag, mIdxPrev, mIdxNext;
  void resetInternal() { mId = mFlag = mIdxPrev = mIdxNext = 0; }

  std::string mName; // Name of this node

  // Offset of data in stream
  // Relative to stream start, not node start. 0 for NULL (root)
  s32 mDataDestination = 0;

  // Adapted from WSZST
  bool calcIdBit(u16 id) const {
    const u32 charIdx = u32(id >> 3);
    return charIdx < mName.size() && (mName[charIdx] >> (id & 7)) & 1;
  }

public:
  //! @brief Serialize a node to or from a stream.
  //!
  //! @param[in] stream			  Stream to read/write to/from.
  //! @param[in] groupStartPosition Position of the Dictionary group in stream.
  //!
  void write(oishii::Writer& stream, u32 groupStartPosition,
             NameTable& names) const;

  // Constructors/destructors
  QDictionaryNode() { resetInternal(); }
  ~QDictionaryNode() {}
  QDictionaryNode(const std::string& name) : mName(name) { resetInternal(); }

  // String getters/setters
  const std::string& getName() const { return mName; }
  std::string& getName() { return mName; }
  void setName(const std::string& str) { mName = str; }

  // Data destination manipulation
  u32 getDataDestination() const { return mDataDestination; }
  void setDataDestination(u32 d) { mDataDestination = d; }
};
class QDictionary {
public:
  std::vector<QDictionaryNode> mNodes{1};
  QDictionaryNode& getRoot() {
    assert(!mNodes.empty());
    return mNodes[0];
  }
  QDictionaryNode& operator[](int idx) {
    assert((int)mNodes.size() > idx && idx > 0);
    return mNodes[idx + 1];
  }

  void emplace(const std::string& name) { mNodes.emplace_back(name); }
  void emplace(const QDictionaryNode& n) { mNodes.emplace_back(n); }
  QDictionaryNode* getAt(const std::string& str) const {
    auto it = std::find_if(
        mNodes.begin(), mNodes.end(),
        [str](const QDictionaryNode& n) { return n.getName() == str; });
    if (it == mNodes.end())
      return 0;
    return const_cast<QDictionaryNode*>(&*it);
  }

private:
  void calcNode(u32 idx);

public:
  // You don't need to call this externally. Write will already do so.
  void calcNodes();

  void write(oishii::Writer& writer, NameTable& names);

  u32 computeSize() const {
    if (mNodes.size() == 0)
      return 0;
    return 8 + 16 * mNodes.size();
  }
};


int CalcDictionarySize(const BetterDictionary& dict) {
  QDictionary tmp;
  for (auto& n : dict.nodes)
    tmp.emplace(n.name);

  return tmp.computeSize();
}
int CalcDictionarySize(int num_nodes) {
  if (num_nodes == 0)
    return 0;
  return 8 + 16 * (num_nodes + 1);
}

Result<BetterDictionary> ReadDictionary(rsl::SafeReader& reader) {
  BetterDictionary result;
  bad::Dictionary dict;
  TRY(dict.read(reader));
  for (std::size_t i = 1; i < dict.mNodes.size(); ++i) {
    const auto& dnode = dict.mNodes[i];
    result.nodes.push_back(BetterNode{
        .name = dnode.mName,
        .stream_pos = static_cast<unsigned int>(dnode.mDataDestination),
    });
  }
  return result;
}

void WriteDictionary(const BetterDictionary& dict,
                            oishii::Writer& writer, NameTable& names) {
  QDictionary tmp;
  for (auto& n : dict.nodes) {
    QDictionaryNode tmp_node;
    tmp_node.setName(n.name);
    tmp_node.setDataDestination(n.stream_pos);
    tmp.emplace(tmp_node);
  }

  // implicitly called by write
  // tmp.calcNodes();
  tmp.write(writer, names);
}


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

Result<void> Dictionary::read(rsl::SafeReader& reader) {
  mNodes.clear();
  const auto grpStart = reader.tell();

  // Note: Entry count does not include the root.
  const auto totalSize = TRY(reader.U32());
  const auto nEntry = TRY(reader.U32());

  for (u32 i = 0; i <= nEntry; i++) {
    DictionaryNode tmp;
    TRY(tmp.read(reader, grpStart));
    mNodes.emplace_back(tmp);
  }

  EXPECT(totalSize == reader.tell() - grpStart);
  return {};
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

Result<void> DictionaryNode::read(rsl::SafeReader& reader,
                                  u32 groupStartPosition) {
  mId = TRY(reader.U16());
  mFlag = TRY(reader.U16());
  mIdxPrev = TRY(reader.U16());
  mIdxNext = TRY(reader.U16());
  mName = TRY(reader.StringOfs(groupStartPosition));

  mDataDestination = groupStartPosition + TRY(reader.S32());
  if (mDataDestination == groupStartPosition)
    mDataDestination = 0;
  return {};
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
