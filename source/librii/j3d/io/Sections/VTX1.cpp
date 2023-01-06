#include "../Sections.hpp"

namespace librii::j3d {

using namespace libcube;

Result<void> readVTX1(BMDOutputContext& ctx) {
  rsl::SafeReader reader(ctx.reader);
  if (!enterSection(ctx, 'VTX1'))
    return std::unexpected("Failed to find VTX1 section");

  ScopedSection g(reader.getUnsafe(), "Vertex Buffers");

  const auto ofsFmt = TRY(reader.S32());
  const auto ofsData = reader.getUnsafe().readX<s32, 13>();

  reader.seekSet(g.start + ofsFmt);
  gx::VertexBufferAttribute type =
      static_cast<gx::VertexBufferAttribute>(TRY(reader.U32()));
  for (; type != gx::VertexBufferAttribute::Terminate;
       type = static_cast<gx::VertexBufferAttribute>(TRY(reader.U32()))) {
    const auto comp = TRY(reader.U32());
    const auto data = TRY(reader.U32());
    // May fail
    const auto gen_data = rsl::enum_cast<gx::VertexBufferType::Generic>(data);
    const auto gen_comp_size =
        (gen_data == gx::VertexBufferType::Generic::f32) ? 4
        : (gen_data == gx::VertexBufferType::Generic::u16 ||
           gen_data == gx::VertexBufferType::Generic::s16)
            ? 2
            : 1;
    const auto shift = TRY(reader.U8());
    reader.getUnsafe().skip(3);

    EXPECT(0 <= shift && shift <= 31);

    auto estride = 0;
    // FIXME: type punning
    void* buf = nullptr;

    VBufferKind bufkind = VBufferKind::position;

    switch (type) {
    case gx::VertexBufferAttribute::Position:
      buf = &ctx.mdl.vertexData.pos;
      bufkind = VBufferKind::position;
      ctx.mdl.vertexData.pos.mQuant = VQuantization{
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::Position>(comp)),
          gx::VertexBufferType(TRY(gen_data)),
          static_cast<u8>(gen_data != gx::VertexBufferType::Generic::f32 ? shift
                                                                         : 0),
          shift,
          static_cast<u8>((comp + 2) * gen_comp_size),
      };
      estride = ctx.mdl.vertexData.pos.mQuant.stride;
      break;
    case gx::VertexBufferAttribute::Normal:
      buf = &ctx.mdl.vertexData.norm;
      bufkind = VBufferKind::normal;
      ctx.mdl.vertexData.norm.mQuant = VQuantization{
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::Normal>(comp)),
          gx::VertexBufferType(TRY(gen_data)),
          static_cast<u8>(gen_data == gx::VertexBufferType::Generic::s8    ? 6
                          : gen_data == gx::VertexBufferType::Generic::s16 ? 14
                                                                           : 0),
          shift,
          static_cast<u8>(3 * gen_comp_size),
      };
      estride = ctx.mdl.vertexData.norm.mQuant.stride;
      break;
    case gx::VertexBufferAttribute::Color0:
    case gx::VertexBufferAttribute::Color1: {
      auto& clr =
          ctx.mdl.vertexData
              .color[static_cast<size_t>(type) -
                     static_cast<size_t>(gx::VertexBufferAttribute::Color0)];
      buf = &clr;
      bufkind = VBufferKind::color;
      clr.mQuant = VQuantization{
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::Color>(comp)),
          gx::VertexBufferType(static_cast<gx::VertexBufferType::Color>(data)),
          0,
          shift,
          TRY([](gx::VertexBufferType::Color c) -> Result<u8> {
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
              EXPECT(false, "Invalid color data type.");
              return 4;
            }
            return {};
          }(static_cast<gx::VertexBufferType::Color>(data))),
      };
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
      auto& uv =
          ctx.mdl.vertexData
              .uv[static_cast<size_t>(type) -
                  static_cast<size_t>(gx::VertexBufferAttribute::TexCoord0)];
      buf = &uv;
      bufkind = VBufferKind::textureCoordinate;
      uv.mQuant = VQuantization{
          gx::VertexComponentCount(
              static_cast<gx::VertexComponentCount::TextureCoordinate>(comp)),
          gx::VertexBufferType(TRY(gen_data)),
          static_cast<u8>(gen_data != gx::VertexBufferType::Generic::f32 ? shift
                                                                         : 0),
          shift,
          static_cast<u8>((comp + 1) * gen_comp_size),
      };
      estride = uv.mQuant.stride;
      break;
    }
    default:
      EXPECT(false, "Unsupported VertexBufferAttribute");
    }

    EXPECT(estride);

    const auto getDataIndex =
        [&](gx::VertexBufferAttribute attr) -> Result<int> {
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

      EXPECT(attr_i < lut.size());

      return attr_i < lut.size() ? lut[attr_i] : -1;
    };

    const auto idx = TRY(getDataIndex(type));
    const auto ofs = g.start + ofsData[idx];
    {
      oishii::Jump<oishii::Whence::Set> g_bufdata(reader.getUnsafe(), ofs);
      s32 size = 0;

      for (int i = idx + 1; i < ofsData.size(); ++i) {
        if (ofsData[i]) {
          size = static_cast<s32>(ofsData[i]) - ofsData[idx];
          break;
        }
      }

      if (!size)
        size = g.size - ofsData[idx];

      EXPECT(size > 0);

      // Desirable to round down
      const auto ensize = (size /*+ estride*/) / estride;
      //	assert(size % estride == 0);
      EXPECT(ensize < u32(u16(-1)) * 3);

      // FIXME: Alignment padding trim

      switch (bufkind) {
      case VBufferKind::position: {
        auto pos = reinterpret_cast<decltype(ctx.mdl.vertexData.pos)*>(buf);

        pos->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i) {
          TRY(pos->readBufferEntryGeneric(reader.getUnsafe(), pos->mData[i]));
        }
        break;
      }
      case VBufferKind::normal: {
        auto nrm = reinterpret_cast<decltype(ctx.mdl.vertexData.norm)*>(buf);

        nrm->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i) {
          TRY(nrm->readBufferEntryGeneric(reader.getUnsafe(), nrm->mData[i]));
        }
        break;
      }
      case VBufferKind::color: {
        auto clr =
            reinterpret_cast<decltype(ctx.mdl.vertexData.color)::value_type*>(
                buf);

        clr->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i) {
          TRY(clr->readBufferEntryColor(reader.getUnsafe(), clr->mData[i]));
        }
        break;
      }
      case VBufferKind::textureCoordinate: {
        auto uv =
            reinterpret_cast<decltype(ctx.mdl.vertexData.uv)::value_type*>(buf);

        uv->mData.resize(ensize);
        for (int i = 0; i < ensize; ++i) {
          TRY(uv->readBufferEntryGeneric(reader.getUnsafe(), uv->mData[i]));
        }
        break;
      }
      }
    }
  }

  return {};
}
struct FormatDecl : public oishii::Node {
  FormatDecl(J3dModel* m) : mdl(m) { mId = "Format"; }
  struct Entry {
    gx::VertexBufferAttribute attrib;
    u32 cnt = 1;
    u32 data = 0;
    u8 shift = 0;

    void write(oishii::Writer& writer) {
      writer.write<u32>(static_cast<u32>(attrib));
      writer.write<u32>(static_cast<u32>(cnt));
      writer.write<u32>(static_cast<u32>(data));
      writer.write<u8>(static_cast<u8>(shift));
      for (int i = 0; i < 3; ++i)
        writer.write<u8>(-1);
    }
  };

  Result write(oishii::Writer& writer) const noexcept {
    // Positions
    if (!mdl->vertexData.pos.mData.empty()) {
      const auto& q = mdl->vertexData.pos.mQuant;
      Entry{gx::VertexBufferAttribute::Position,
            static_cast<u32>(q.comp.position), static_cast<u32>(q.type.generic),
            q.bad_divisor}
          .write(writer);
    }
    // Normals
    if (!mdl->vertexData.norm.mData.empty()) {
      const auto& q = mdl->vertexData.norm.mQuant;
      Entry{gx::VertexBufferAttribute::Normal, static_cast<u32>(q.comp.normal),
            static_cast<u32>(q.type.generic), q.bad_divisor}
          .write(writer);
    }
    // Colors
    {
      int i = 0;
      for (const auto& buf : mdl->vertexData.color) {
        if (!buf.mData.empty()) {
          const auto& q = buf.mQuant;
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
      for (const auto& buf : mdl->vertexData.uv) {
        if (!buf.mData.empty()) {
          const auto& q = buf.mQuant;
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
  J3dModel* mdl;
};

struct VTX1Node {
  static const char* getNameId() { return "VTX1"; }
  virtual const oishii::Node& getSelf() const = 0;

  int computeNumOfs() const {
    int numOfs = 0;
    if (mdl) {
      if (!mdl->vertexData.pos.mData.empty())
        ++numOfs;
      if (!mdl->vertexData.norm.mData.empty())
        ++numOfs;
      for (int i = 0; i < 2; ++i)
        if (!mdl->vertexData.color[i].mData.empty())
          ++numOfs;
      for (int i = 0; i < 8; ++i)
        if (!mdl->vertexData.uv[i].mData.empty())
          ++numOfs;
    }
    return numOfs;
  }

  void write(oishii::Writer& writer) const {
    if (!mdl)
      return;

    writer.write<u32, oishii::EndianSelect::Big>('VTX1');
    writer.writeLink<s32>(
        oishii::Link{oishii::Hook(getSelf()),
                     oishii::Hook(getSelf(), oishii::Hook::EndOfChildren)});

    writer.writeLink<s32>(
        oishii::Link{oishii::Hook(getSelf()), oishii::Hook("Format")});

    // const auto numOfs = computeNumOfs();
    int i = 0;
    auto writeBufLink = [&]() {
      writer.writeLink<s32>(oishii::Link{
          oishii::Hook(getSelf()), oishii::Hook("Buf" + std::to_string(i++))});
    };
    auto writeOptBufLink = [&](const auto& b) {
      if (b.mData.empty())
        writer.write<s32>(0);
      else
        writeBufLink();
    };

    writeOptBufLink(mdl->vertexData.pos);
    writeOptBufLink(mdl->vertexData.norm);
    writer.write<s32>(0); // NBT

    for (const auto& c : mdl->vertexData.color)
      writeOptBufLink(c);

    for (const auto& u : mdl->vertexData.uv)
      writeOptBufLink(u);
  }

  template <typename T> struct VertexAttribBuf : public oishii::Node {
    VertexAttribBuf(const J3dModel& m, const std::string& id, const T& data)
        : Node(id), mdl(m), mData(data) {
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 32;
    }

    Result write(oishii::Writer& writer) const noexcept {
      if (mData.writeData(writer))
        return eResult::Success;

      return eResult::Fatal;
    }

    const J3dModel& mdl;
    const T& mData;
  };

  void gatherChildren(oishii::Node::NodeDelegate& ctx) const {

    ctx.addNode(std::make_unique<FormatDecl>(mdl));

    int i = 0;

    auto push_buf = [&](auto& buf) {
      ctx.addNode(std::make_unique<VertexAttribBuf<decltype(buf)>>(
          *mdl, "Buf" + std::to_string(i++), buf));
    };

    // Positions
    if (!mdl->vertexData.pos.mData.empty())
      push_buf(mdl->vertexData.pos);

    // Normals
    if (!mdl->vertexData.norm.mData.empty())
      push_buf(mdl->vertexData.norm);

    // Colors
    for (auto& c : mdl->vertexData.color)
      if (!c.mData.empty())
        push_buf(c);

    // UV
    for (auto& c : mdl->vertexData.uv)
      if (!c.mData.empty())
        push_buf(c);
  }

  VTX1Node(BMDExportContext& ctx) : mdl(&ctx.mdl) {}
  J3dModel* mdl = nullptr;
};

std::unique_ptr<oishii::Node> makeVTX1Node(BMDExportContext& ctx) {
  return std::make_unique<LinkNode<VTX1Node>>(ctx);
}

} // namespace librii::j3d
