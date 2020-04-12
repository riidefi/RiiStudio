#include "../Sections.hpp"

namespace riistudio::j3d {

using namespace libcube;

void readVTX1(BMDOutputContext &ctx) {
  auto &reader = ctx.reader;
  if (!enterSection(ctx, 'VTX1'))
    return;

  ScopedSection g(reader, "Vertex Buffers");

  const auto ofsFmt = reader.read<s32>();
  const auto ofsData = reader.readX<s32, 13>();

  reader.seekSet(g.start + ofsFmt);
  for (gx::VertexBufferAttribute type =
           reader.read<gx::VertexBufferAttribute>();
       type != gx::VertexBufferAttribute::Terminate;
       type = reader.read<gx::VertexBufferAttribute>()) {
    const auto comp = reader.read<u32>();
    const auto data = reader.read<u32>();
    const auto gen_data = static_cast<gx::VertexBufferType::Generic>(data);
    const auto gen_comp_size =
        (gen_data == gx::VertexBufferType::Generic::f32)
            ? 4
            : (gen_data == gx::VertexBufferType::Generic::u16 ||
               gen_data == gx::VertexBufferType::Generic::s16)
                  ? 2
                  : 1;
    const auto shift = reader.read<u8>();
    reader.seek(3);

    assert(0 <= shift && shift <= 31);

    auto estride = 0;
    // FIXME: type punning
    void *buf = nullptr;

    VBufferKind bufkind = VBufferKind::undefined;

    switch (type) {
    case gx::VertexBufferAttribute::Position:
      buf = &ctx.mdl.get().mBufs.pos;
      bufkind = VBufferKind::position;
      ctx.mdl.get().mBufs.pos.mQuant = VQuantization(
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::Position>(comp)),
          gx::VertexBufferType(gen_data),
          gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0, shift,
          (comp + 2) * gen_comp_size);
      estride = ctx.mdl.get().mBufs.pos.mQuant.stride;
      break;
    case gx::VertexBufferAttribute::Normal:
      buf = &ctx.mdl.get().mBufs.norm;
      bufkind = VBufferKind::normal;
      ctx.mdl.get().mBufs.norm.mQuant = VQuantization(
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::Normal>(comp)),
          gx::VertexBufferType(gen_data),
          gen_data == gx::VertexBufferType::Generic::s8
              ? 6
              : gen_data == gx::VertexBufferType::Generic::s16 ? 14 : 0,
          shift, 3 * gen_comp_size);
      estride = ctx.mdl.get().mBufs.norm.mQuant.stride;
      break;
    case gx::VertexBufferAttribute::Color0:
    case gx::VertexBufferAttribute::Color1: {
      auto &clr =
          ctx.mdl.get().mBufs.color[static_cast<size_t>(type) -
                                    static_cast<size_t>(
                                        gx::VertexBufferAttribute::Color0)];
      buf = &clr;
      bufkind = VBufferKind::color;
      clr.mQuant = VQuantization(
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::Color>(comp)),
          gx::VertexBufferType(static_cast<gx::VertexBufferType::Color>(data)),
          0, shift, [](gx::VertexBufferType::Color c) {
            using ct = gx::VertexBufferType::Color;
            switch (c) {
            case ct::FORMAT_16B_4444:
            case ct::FORMAT_16B_565:
              return 2;
            case ct::FORMAT_24B_6666:
            case ct::FORMAT_24B_888:
              return 3;
            case ct::FORMAT_32B_8888:
            case ct::FORMAT_32B_888x:
              return 4;
            default:
              throw "Invalid color data type.";
            }
          }(static_cast<gx::VertexBufferType::Color>(data)));
      estride = clr.mQuant.stride;
      break;
    }
    case gx::VertexBufferAttribute::TexCoord0:
    case gx::VertexBufferAttribute::TexCoord1:
    case gx::VertexBufferAttribute::TexCoord2:
    case gx::VertexBufferAttribute::TexCoord3:
    case gx::VertexBufferAttribute::TexCoord4:
    case gx::VertexBufferAttribute::TexCoord5:
    case gx::VertexBufferAttribute::TexCoord6:
    case gx::VertexBufferAttribute::TexCoord7: {
      auto &uv =
          ctx.mdl.get().mBufs.uv[static_cast<size_t>(type) -
                                 static_cast<size_t>(
                                     gx::VertexBufferAttribute::TexCoord0)];
      buf = &uv;
      bufkind = VBufferKind::textureCoordinate;
      uv.mQuant = VQuantization(
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::TextureCoordinate>(comp)),
          gx::VertexBufferType(gen_data),
          gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0, shift,
          (comp + 1) * gen_comp_size);
      estride = uv.mQuant.stride;
      break;
    }
    default:
      assert(false);
    }

    assert(estride);
    assert(bufkind != VBufferKind::undefined);

    const auto getDataIndex = [&](gx::VertexBufferAttribute attr) {
      static const constexpr std::array<
          s8, static_cast<size_t>(
                  gx::VertexBufferAttribute::NormalBinormalTangent) +
                  1>
          lut = {-1, -1, -1, -1, -1, -1, -1, -1, -1,
                 0,             // Position
                 1,             // Normal
                 3,  4,         // Color
                 5,  6,  7,  8, // UV
                 9,  10, 11, 12, -1, -1, -1, -1, 2};

      static_assert(lut[static_cast<size_t>(
                        gx::VertexBufferAttribute::NormalBinormalTangent)] == 2,
                    "NBT");

      const auto attr_i = static_cast<size_t>(attr);

      assert(attr_i < lut.size());

      return attr_i < lut.size() ? lut[attr_i] : -1;
    };

    const auto idx = getDataIndex(type);
    const auto ofs = g.start + ofsData[idx];
    {
      oishii::Jump<oishii::Whence::Set> g_bufdata(reader, ofs);
      s32 size = 0;

      for (int i = idx + 1; i < ofsData.size(); ++i) {
        if (ofsData[i]) {
          size = static_cast<s32>(ofsData[i]) - ofsData[idx];
          break;
        }
      }

      if (!size)
        size = g.size - ofsData[idx];

      assert(size > 0);

      // Desirable to round down
      const auto ensize = (size /*+ estride*/) / estride;
      //	assert(size % estride == 0);
      assert(ensize < u32(u16(-1)) * 3);

      // FIXME: Alignment padding trim

      switch (bufkind) {
      case VBufferKind::position: {
        auto pos = reinterpret_cast<decltype(ctx.mdl.get().mBufs.pos) *>(buf);

        pos->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i)
          pos->readBufferEntryGeneric(reader, pos->mData[i]);
        break;
      }
      case VBufferKind::normal: {
        auto nrm = reinterpret_cast<decltype(ctx.mdl.get().mBufs.norm) *>(buf);

        nrm->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i)
          nrm->readBufferEntryGeneric(reader, nrm->mData[i]);
        break;
      }
      case VBufferKind::color: {
        auto clr =
            reinterpret_cast<decltype(ctx.mdl.get().mBufs.color)::value_type *>(
                buf);

        clr->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i)
          clr->readBufferEntryColor(reader, clr->mData[i]);
        break;
      }
      case VBufferKind::textureCoordinate: {
        auto uv =
            reinterpret_cast<decltype(ctx.mdl.get().mBufs.uv)::value_type *>(
                buf);

        uv->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i)
          uv->readBufferEntryGeneric(reader, uv->mData[i]);
        break;
      }
      }
    }
  }
}
struct FormatDecl : public oishii::v2::Node {
  FormatDecl(Model *m) : mdl(m) { mId = "Format"; }
  struct Entry {
    gx::VertexBufferAttribute attrib;
    u32 cnt = 1;
    u32 data = 0;
    u8 shift = 0;

    void write(oishii::v2::Writer &writer) {
      writer.write<u32>(static_cast<u32>(attrib));
      writer.write<u32>(static_cast<u32>(cnt));
      writer.write<u32>(static_cast<u32>(data));
      writer.write<u8>(static_cast<u8>(shift));
      for (int i = 0; i < 3; ++i)
        writer.write<u8>(-1);
    }
  };

  Result write(oishii::v2::Writer &writer) const noexcept {
    // Positions
    if (!mdl->mBufs.pos.mData.empty()) {
      const auto &q = mdl->mBufs.pos.mQuant;
      Entry{gx::VertexBufferAttribute::Position,
            static_cast<u32>(q.comp.position), static_cast<u32>(q.type.generic),
            q.bad_divisor}
          .write(writer);
    }
    // Normals
    if (!mdl->mBufs.norm.mData.empty()) {
      const auto &q = mdl->mBufs.norm.mQuant;
      Entry{gx::VertexBufferAttribute::Normal, static_cast<u32>(q.comp.normal),
            static_cast<u32>(q.type.generic), q.bad_divisor}
          .write(writer);
    }
    // Colors
    {
      int i = 0;
      for (const auto &buf : mdl->mBufs.color) {
        if (!buf.mData.empty()) {
          const auto &q = buf.mQuant;
          Entry{gx::VertexBufferAttribute(
                    (int)gx::VertexBufferAttribute::Color0 + i),
                static_cast<u32>(q.comp.color), static_cast<u32>(q.type.color),
                q.bad_divisor}
              .write(writer);
        }
        ++i;
      }
    }
    // UVs
    {
      int i = 0;
      for (const auto &buf : mdl->mBufs.uv) {
        if (!buf.mData.empty()) {
          const auto &q = buf.mQuant;
          Entry{gx::VertexBufferAttribute(
                    (int)gx::VertexBufferAttribute::TexCoord0 + i),
                static_cast<u32>(q.comp.texcoord),
                static_cast<u32>(q.type.generic), q.bad_divisor}
              .write(writer);
        }
        ++i;
      }
    }
    Entry{gx::VertexBufferAttribute::Terminate}.write(writer);
    return {};
  }
  Model *mdl;
};

struct VTX1Node {
  static const char *getNameId() { return "VTX1"; }
  virtual const oishii::v2::Node &getSelf() const = 0;

  int computeNumOfs() const {
    int numOfs = 0;
    if (mdl) {
      if (!mdl->mBufs.pos.mData.empty())
        ++numOfs;
      if (!mdl->mBufs.norm.mData.empty())
        ++numOfs;
      for (int i = 0; i < 2; ++i)
        if (!mdl->mBufs.color[i].mData.empty())
          ++numOfs;
      for (int i = 0; i < 8; ++i)
        if (!mdl->mBufs.uv[i].mData.empty())
          ++numOfs;
    }
    return numOfs;
  }

  void write(oishii::v2::Writer &writer) const {
    if (!mdl)
      return;

    writer.write<u32, oishii::EndianSelect::Big>('VTX1');
    writer.writeLink<s32>(oishii::v2::Link{
        oishii::v2::Hook(getSelf()),
        oishii::v2::Hook(getSelf(), oishii::v2::Hook::EndOfChildren)});

    writer.writeLink<s32>(oishii::v2::Link{oishii::v2::Hook(getSelf()),
                                           oishii::v2::Hook("Format")});

    const auto numOfs = computeNumOfs();
    int i = 0;
    auto writeBufLink = [&]() {
      writer.writeLink<s32>(
          oishii::v2::Link{oishii::v2::Hook(getSelf()),
                           oishii::v2::Hook("Buf" + std::to_string(i++))});
    };
    auto writeOptBufLink = [&](const auto &b) {
      if (b.mData.empty())
        writer.write<s32>(0);
      else
        writeBufLink();
    };

    writeOptBufLink(mdl->mBufs.pos);
    writeOptBufLink(mdl->mBufs.norm);
    writer.write<s32>(0); // NBT

    for (const auto &c : mdl->mBufs.color)
      writeOptBufLink(c);

    for (const auto &u : mdl->mBufs.uv)
      writeOptBufLink(u);
  }

  template <typename T> struct VertexAttribBuf : public oishii::v2::Node {
    VertexAttribBuf(const Model &m, const std::string &id, const T &data)
        : Node(id), mdl(m), mData(data) {
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 32;
    }

    Result write(oishii::v2::Writer &writer) const noexcept {
      mData.writeData(writer);
      return eResult::Success;
    }

    const Model &mdl;
    const T &mData;
  };

  void gatherChildren(oishii::v2::Node::NodeDelegate &ctx) const {

    ctx.addNode(std::make_unique<FormatDecl>(mdl));

    int i = 0;

    auto push_buf = [&](auto &buf) {
      ctx.addNode(std::make_unique<VertexAttribBuf<decltype(buf)>>(
          *mdl, "Buf" + std::to_string(i++), buf));
    };

    // Positions
    if (!mdl->mBufs.pos.mData.empty())
      push_buf(mdl->mBufs.pos);

    // Normals
    if (!mdl->mBufs.norm.mData.empty())
      push_buf(mdl->mBufs.norm);

    // Colors
    for (auto &c : mdl->mBufs.color)
      if (!c.mData.empty())
        push_buf(c);

    // UV
    for (auto &c : mdl->mBufs.uv)
      if (!c.mData.empty())
        push_buf(c);
  }

  VTX1Node(BMDExportContext &ctx) : mdl(&ctx.mdl.get()) {}
  Model *mdl = nullptr;
};

std::unique_ptr<oishii::v2::Node> makeVTX1Node(BMDExportContext &ctx) {
  return std::make_unique<LinkNode<VTX1Node>>(ctx);
}

} // namespace riistudio::j3d
