#pragma once

#include <core/util/glm_io.hpp>
#include <functional>
#include <librii/g3d/io/TextureIO.hpp>
#include <librii/gx.h>
#include <llvm/ADT/SmallVector.h>
#include <map>
#include <oishii/writer/binary_writer.hxx>
#include <plugins/g3d/util/Dictionary.hpp>
#include <plugins/g3d/util/NameTable.hpp>
#include <string>

inline std::span<const u8> SliceStream(oishii::BinaryReader& reader) {
  return {reader.getStreamStart() + reader.tell(),
          reader.endpos() - reader.tell()};
}

inline void operator<<(librii::gx::Color& out, oishii::BinaryReader& reader) {
  out = librii::gx::readColorComponents(
      reader, librii::gx::VertexBufferType::Color::rgba8);
}

inline void operator>>(const librii::gx::Color& out, oishii::Writer& writer) {
  librii::gx::writeColorComponents(writer, out,
                                   librii::gx::VertexBufferType::Color::rgba8);
}

inline auto writePlaceholder(oishii::Writer& writer) {
  writer.write<s32>(0);
  return writer.tell() - 4;
}
inline void writeOffsetBackpatch(oishii::Writer& w, std::size_t pointer,
                                 std::size_t from) {
  auto old = w.tell();
  w.seekSet(pointer);
  w.write<s32>(old - from);
  w.seekSet(old);
}

namespace riistudio::g3d {

class RelocWriter {
public:
  struct Reloc {
    std::string from, to;
    std::size_t ofs, sz;
  };

  RelocWriter(oishii::Writer& writer) : mWriter(writer) {}
  RelocWriter(oishii::Writer& writer, const std::string& prefix)
      : mPrefix(prefix), mWriter(writer) {}

  RelocWriter sublet(const std::string& path) {
    return RelocWriter(mWriter, mPrefix + "/" + path);
  }

  // Define a label, associated with the current stream position
  void label(const std::string& id, const std::size_t addr) {
    mLabels.try_emplace(id, addr);
  }
  void label(const std::string& id) { label(id, mWriter.tell()); }
  // Write a relocation, to be filled in by a resolve() call
  template <typename T>
  void writeReloc(const std::string& from, const std::string& to) {
    mRelocs.emplace_back(
        Reloc{.from = from, .to = to, .ofs = mWriter.tell(), .sz = sizeof(T)});
    mWriter.write<T>(static_cast<T>(0));
  }
  // Write a relocation, with an associated child frame to be written by
  // writeChildren()
  template <typename T>
  void writeReloc(const std::string& from, const std::string& to,
                  std::function<void(oishii::Writer&)> write) {
    mRelocs.emplace_back(
        Reloc{.from = from, .to = to, .ofs = mWriter.tell(), .sz = sizeof(T)});
    mWriter.write<T>(static_cast<T>(-1));
    mChildren.emplace_back(to, write);
  }
  // Write all child bodies
  void writeChildren() {
    for (auto& child : mChildren) {
      label(child.first);
      child.second(mWriter);
    }
    mChildren.clear();
  }
  // Resolve a single relocation
  void resolve(Reloc& reloc) {
    const auto from = mLabels.find(reloc.from);
    const auto to = mLabels.find(reloc.to);

    int delta = 0;
    if (from == mLabels.end() || to == mLabels.end()) {
      printf("Bad lookup: %s to %s\n", reloc.from.c_str(), reloc.to.c_str());
      return; // come back..
    } else {
      delta = to->second - from->second;
    }

    const auto back = mWriter.tell();

    mWriter.seekSet(reloc.ofs);

    if (reloc.sz == 4) {
      mWriter.write<s32>(delta);
    } else if (reloc.sz == 2) {
      mWriter.write<s16>(delta);
    } else {
      assert(false);
      printf("Invalid reloc size..\n");
    }

    mWriter.seekSet(back);
  }
  // Resolve all relocations
  void resolve() {
    for (auto& reloc : mRelocs)
      resolve(reloc);
    mRelocs.erase(
        std::remove_if(mRelocs.begin(), mRelocs.end(), [&](auto& reloc) {
          const auto from = mLabels.find(reloc.from);
          const auto to = mLabels.find(reloc.to);

          return from != mLabels.end() && to != mLabels.end();
        }));
  }

  auto& getLabels() const { return mLabels; }
  void printLabels() const {
    for (auto& [label, at] : mLabels) {
      const auto uat = static_cast<unsigned>(at);
      printf("%s: 0x%x (%u)\n", label.c_str(), uat, uat);
    }
  }

private:
  std::string mPrefix;

  oishii::Writer& mWriter;
  std::map<std::string, std::size_t> mLabels;

  llvm::SmallVector<Reloc, 64> mRelocs;
  llvm::SmallVector<
      std::pair<std::string, std::function<void(oishii::Writer&)>>, 16>
      mChildren;
};

template <bool Named, bool bMaterial, typename T, typename U>
void writeDictionary(const std::string& name, T src_range, U handler,
                     RelocWriter& linker, oishii::Writer& writer, u32 mdl_start,
                     NameTable& names, int* d_cursor, bool raw = false,
                     u32 align = 4, bool BetterMethod = false) {
  if (src_range.size() == 0)
    return;

  u32 dict_ofs;
  QDictionary _dict;
  _dict.mNodes.resize(src_range.size() + 1);

  if (BetterMethod) {
    dict_ofs = writer.tell();
  } else {
    dict_ofs = *d_cursor;
    *d_cursor += _dict.computeSize();
  }

  if (BetterMethod)
    writer.skip(_dict.computeSize());

  for (std::size_t i = 0; i < src_range.size(); ++i) {
    writer.alignTo(align); //
    _dict.mNodes[i + 1].setDataDestination(writer.tell());
    if constexpr (Named) {
      if constexpr (bMaterial)
        _dict.mNodes[i + 1].setName(src_range[i].IGCMaterial::getName());
      else
        _dict.mNodes[i + 1].setName(src_range[i].getName());
    }

    if (!raw) {
      const auto backpatch = writePlaceholder(writer); // size
      writer.write<s32>(mdl_start - backpatch);        // mdl0 offset
      handler(src_range[i], backpatch);
      writeOffsetBackpatch(writer, backpatch, backpatch);
    } else {
      handler(src_range[i], writer.tell());
    }
  }
  {
    oishii::Jump<oishii::Whence::Set, oishii::Writer> d(writer, dict_ofs);
    linker.label(name);
    _dict.write(writer, names);
  }
};

const std::array<u32, 16> shaderDlSizes{
    160, //  0
    160, //  1
    192, //  2
    192, //  3
    256, //  4
    256, //  5
    288, //  6
    288, //  7
    352, //  8
    352, //  9
    384, // 10
    384, // 11
    448, // 12
    448, // 13
    480, // 14
    480, // 15
};

struct DlHandle {
  std::size_t tag_start;
  std::size_t buf_size = 0;
  std::size_t cmd_size = 0;
  s32 ofs_buf = 0;
  oishii::Writer* mWriter = nullptr;

  void seekTo(oishii::BinaryReader& reader) {
    reader.seekSet(tag_start + ofs_buf);
  }
  DlHandle(oishii::BinaryReader& reader) : tag_start(reader.tell()) {
    buf_size = reader.read<u32>();
    cmd_size = reader.read<u32>();
    ofs_buf = reader.read<s32>();
  }
  DlHandle(oishii::Writer& writer)
      : tag_start(writer.tell()), mWriter(&writer) {
    mWriter->skip(4 * 3);
  }
  void write() {
    assert(mWriter != nullptr);
    mWriter->writeAt<u32>(buf_size, tag_start);
    mWriter->writeAt<u32>(cmd_size, tag_start + 4);
    mWriter->writeAt<u32>(ofs_buf, tag_start + 8);
  }
  // Buf size implied
  void setCmdSize(std::size_t c) {
    cmd_size = c;
    buf_size = roundUp(c, 32);
  }
  void setBufSize(std::size_t c) { buf_size = c; }
  void setBufAddr(s32 addr) { ofs_buf = addr - tag_start; }
};

enum class RenderCommand {
  NoOp,
  Return,

  NodeDescendence,
  NodeMixing,

  Draw,

  EnvelopeMatrix,
  MatrixCopy
};

struct RelocationToApply {
  RelocationToApply(NameTable& table, oishii::Writer& writer, u32 start)
      : mTable(table), mWriter(writer), mStart(start) {}
  ~RelocationToApply() { apply(); }

  operator librii::g3d::NameReloc&() { return mReloc; }

private:
  void apply() {
    mReloc.offset_of_delta_reference += mStart;
    mReloc.offset_of_pointer_in_struct += mStart;
    ApplyGlobalRelocToOishii(mTable, mWriter, mReloc);
  }

  NameTable& mTable;
  oishii::Writer& mWriter;
  u32 mStart;
  librii::g3d::NameReloc mReloc;
};

inline std::pair<u32, std::span<u8>> HandleBlock(oishii::Writer& writer,
                                                 librii::g3d::BlockData block) {
  writer.alignTo(block.start_align);
  const auto start_addr = writer.reserveNext(block.size);
  writer.seekSet(start_addr + block.size);

  std::span<u8> span(writer.getDataBlockStart() + start_addr, block.size);
  return {start_addr, span};
}

} // namespace riistudio::g3d
