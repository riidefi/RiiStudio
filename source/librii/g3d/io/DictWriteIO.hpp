#pragma once

#include <algorithm>
#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <string>
#include <vector>

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

  void read(oishii::BinaryReader& reader, u32 start);
  void write(oishii::Writer& writer, u32 groupStartPosition,
             NameTable& table) const;

  DictionaryNode() = default;
  DictionaryNode(const std::string& name) : mName(name) {}
  DictionaryNode(oishii::BinaryReader& reader, u32 start) {
    read(reader, start);
  }
};

struct Dictionary {
  std::vector<DictionaryNode> mNodes;

  void calcNode(u32 id);
  void calcNodes();

  void read(oishii::BinaryReader& reader);
  void write(oishii::Writer& writer, NameTable& table);

  Dictionary() : mNodes(1){};
  Dictionary(oishii::BinaryReader& reader) { read(reader); }
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
  void read(oishii::BinaryReader& stream, u32 groupStartPosition);

  // Constructors/destructors
  QDictionaryNode() { resetInternal(); }
  ~QDictionaryNode() {}
  QDictionaryNode(const std::string& name) : mName(name) { resetInternal(); }
  QDictionaryNode(oishii::BinaryReader& reader, u32 grpOfs) {
    resetInternal();
    read(reader, grpOfs);
  }

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

  // Cursor API
  int cursor = 1; // Past root
  QDictionaryNode& cGet();

private:
  void calcNode(u32 idx);

public:
  // You don't need to call this externally. Write will already do so.
  void calcNodes();

  void read(oishii::BinaryReader& reader);

  void write(oishii::Writer& writer, NameTable& names);

  u32 computeSize() const {
    if (mNodes.size() == 0)
      return 0;
    return 8 + 16 * mNodes.size();
  }
};

// Only exposes what you need. Use this.
struct BetterNode {
  std::string name;
  unsigned int stream_pos;
};

struct BetterDictionary {
  std::vector<BetterNode> nodes;

  BetterDictionary() = default;
  BetterDictionary(const BetterDictionary&) = default;
  BetterDictionary(BetterDictionary&&) = default;
  BetterDictionary(size_t n) : nodes(n) {}

  bool operator==(const BetterDictionary&) const = default;
};

inline int CalcDictionarySize(const BetterDictionary& dict) {
  QDictionary tmp;
  for (auto& n : dict.nodes)
    tmp.emplace(n.name);

  return tmp.computeSize();
}
inline int CalcDictionarySize(int num_nodes) {
  if (num_nodes == 0)
    return 0;
  return 8 + 16 * (num_nodes + 1);
}

inline void WriteDictionary(const BetterDictionary& dict,
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

} // namespace librii::g3d
