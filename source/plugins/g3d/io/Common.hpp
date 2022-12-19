#pragma once

#include <core/util/glm_io.hpp>
#include <functional>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>
#include <librii/gx.h>
#include <llvm/ADT/SmallVector.h>
#include <map>
#include <oishii/writer/binary_writer.hxx>
#include <plugins/g3d/util/NameTable.hpp>
#include <string>


inline void operator<<(librii::gx::Color& out, oishii::BinaryReader& reader) {
  out = librii::gx::readColorComponents(
      reader, librii::gx::VertexBufferType::Color::rgba8);
}

inline void operator>>(const librii::gx::Color& out, oishii::Writer& writer) {
  librii::gx::writeColorComponents(writer, out,
                                   librii::gx::VertexBufferType::Color::rgba8);
}

namespace riistudio::g3d {

using RelocWriter = librii::g3d::RelocWriter;

template <bool Named, bool bMaterial, typename T, typename U>
void writeDictionary(const std::string& name, T&& src_range, U handler,
                     RelocWriter& linker, oishii::Writer& writer, u32 mdl_start,
                     NameTable& names, int* d_cursor, bool raw = false,
                     u32 align = 4, bool BetterMethod = false) {
  if (src_range.size() == 0)
    return;

  u32 dict_ofs;
  librii::g3d::QDictionary _dict;
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
        _dict.mNodes[i + 1].setName(src_range[i].name);
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
