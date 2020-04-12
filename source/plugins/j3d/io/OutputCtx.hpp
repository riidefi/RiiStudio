#pragma once

#include <core/common.h>
#include <map>
#include <oishii/v2/writer/binary_writer.hxx>
#include <oishii/v2/writer/node.hxx>
#include <plugins/j3d/Model.hpp>
#include <plugins/j3d/Scene.hpp>
#include <vector>

namespace riistudio::j3d {

struct BMDOutputContext {
  ModelAccessor mdl;
  CollectionAccessor col;

  oishii::BinaryReader& reader;

  // Compression ID LUT (remap table)
  std::vector<u16> jointIdLut;
  std::vector<u16> materialIdLut;
  std::vector<u16> shapeIdLut;

  // For VTX1 trimming (length isn't stored)
  std::map<libcube::gx::VertexBufferAttribute, u32>
      mVertexBufferMaxIndices; // Attr : Max Idx

  // Associate section magics with file positions and size
  struct SectionEntry {
    std::size_t streamPos;
    u32 size;
  };
  std::map<u32, SectionEntry> mSections;
};

template <typename T, u32 r, u32 c, typename TM, glm::qualifier qM>
inline void transferMatrix(glm::mat<r, c, TM, qM>& mat, T& stream) {
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j)
      stream.transfer(mat[j][i]);
}
template <typename T, u32 r, u32 c, typename TM, glm::qualifier qM>
inline void writeMatrix(const glm::mat<r, c, TM, qM>& mat, T& stream) {
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j)
      stream.write(mat[j][i]);
}

inline std::vector<std::string> readNameTable(oishii::BinaryReader& reader) {
  const auto start = reader.tell();
  std::vector<std::string> collected(reader.read<u16>());
  reader.read<u16>();

  for (auto& e : collected) {
    const auto [hash, ofs] = reader.readX<u16, 2>();
    {
      oishii::Jump<oishii::Whence::Set> g(reader, start + ofs);

      for (char c = reader.read<s8>(); c; c = reader.read<s8>())
        e.push_back(c);
    }
  }

  return collected;
}

inline u16 hash(const std::string& str) {
  u16 digest = 0;
  for (const char c : str)
    digest = digest * 3 + c;
  return digest;
}

inline void writeNameTable(oishii::v2::Writer& writer,
                           const std::vector<std::string>& names) {
  u32 start = writer.tell();
  writer.write<u16>(static_cast<u16>(names.size()));
  writer.write<u16>(0xffff);

  writer.seek<oishii::Whence::Current>(names.size() * 4);
  int i = 0;
  for (const auto& str : names) {
    const u32 strStart = writer.tell();
    for (const char c : str)
      if (c == 0)
        printf("???\n");
      else
        writer.write<u8>(c);
    writer.write<u8>(0);
    // const u32 end = writer.tell();

    {
      oishii::Jump<oishii::Whence::Set, oishii::v2::Writer> g(
          writer, start + 4 + i * 4);

      writer.write<u16>(hash(str));
      writer.write<u16>(strStart - start);
    }
    ++i;
  }
  // (hash, ofs)*
}

inline bool enterSection(BMDOutputContext& ctx, u32 id) {
  const auto sec = ctx.mSections.find(id);
  if (sec == ctx.mSections.end())
    return false;

  ctx.reader.seekSet(sec->second.streamPos - 8);
  return true;
}

struct ScopedSection : private oishii::BinaryReader::ScopedRegion {
  ScopedSection(oishii::BinaryReader& reader, const char* name)
      : oishii::BinaryReader::ScopedRegion(reader, name) {
    start = reader.tell();
    reader.seek(4);
    size = reader.read<u32>();
  }
  u32 start = 0;
  u32 size = 0;
};

template <typename T, bool leaf = false>
struct LinkNode final : public T, public oishii::v2::Node {

  template <typename... S>
  LinkNode(S... arg) : T(arg...), Node(T::getNameId()) {
    if (leaf)
      mLinkingRestriction.setFlag(oishii::v2::LinkingRestriction::Leaf);
  }
  oishii::v2::Node::Result write(oishii::v2::Writer& writer) const
      noexcept override {
    T::write(writer);
    return eResult::Success;
  }

  oishii::v2::Node::Result
  gatherChildren(oishii::v2::Node::NodeDelegate& out) const noexcept override {
    T::gatherChildren(out);

    return {};
  }
  const oishii::v2::Node& getSelf() const override { return *this; }
};

struct BMDExportContext {
  ModelAccessor mdl;
  CollectionAccessor col;
  /*
  We need to associate Samplers and TexData
  */
};

} // namespace riistudio::j3d
