#include "PolygonIO.hpp"
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLInterpreter.hpp>
#include <rsl/ArrayUtil.hpp>

namespace librii::g3d {

struct DlHandle {
  std::size_t tag_start{}; // reader.tell()
  std::size_t buf_size = 0;
  std::size_t cmd_size = 0;
  s32 ofs_buf = 0;
  oishii::Writer* mWriter = nullptr;

  void seekTo(rsl::SafeReader& reader) { reader.seekSet(tag_start + ofs_buf); }
  static Result<DlHandle> make(rsl::SafeReader& reader) {
    DlHandle tmp;
    tmp.tag_start = reader.tell();
    tmp.buf_size = TRY(reader.U32());
    tmp.cmd_size = TRY(reader.U32());
    tmp.ofs_buf = TRY(reader.S32());
    return tmp;
  }
  DlHandle() = default;
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

struct DLCache {
  std::array<u8, 5 * 2> runtime_data{};
  librii::gx::VertexDescriptor descv;

  void write(oishii::Writer& writer) {
    librii::gpu::DLBuilder dl(writer);
    writer.skip(5 * 2); // runtime
    dl.setVtxDescv(descv);
    writer.write<u8>(0);
    // dl.align(); // 1
  }
};

struct VertexDescriptor {
  std::vector<std::pair<librii::gx::VertexAttribute, librii::gx::VQuantization>>
      desc;
  Result<void> Build(const librii::gx::VertexDescriptor& d,
                     const librii::g3d::BinaryModel& mdl,
                     const librii::g3d::PolygonData& mesh) {
    for (auto [attr, type] : d.mAttributes) {
      if (type == librii::gx::VertexAttributeType::None)
        continue;
      librii::gx::VQuantization quant;

      const auto set_quant = [&](const auto& quantize) {
        quant.comp = quantize.mComp;
        quant.type = quantize.mType;
        quant.divisor = quantize.divisor;
        quant.bad_divisor = quantize.divisor;
        quant.stride = quantize.stride;
      };

      using VA = librii::gx::VertexAttribute;
      switch (attr) {
      case VA::Position: {
        const auto* buf = findByName(mdl.positions, mesh.mPositionBuffer);
        if (!buf) {
          return std::unexpected("Cannot find position buffer " +
                                 mesh.mPositionBuffer);
        }
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color0: {
        const auto* buf = findByName(mdl.colors, mesh.mColorBuffer[0]);
        if (!buf) {
          return std::unexpected("Cannot find color buffer " +
                                 mesh.mColorBuffer[0]);
        }
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color1: {
        const auto* buf = findByName(mdl.colors, mesh.mColorBuffer[1]);
        if (!buf) {
          return std::unexpected("Cannot find color buffer " +
                                 mesh.mColorBuffer[1]);
        }
        set_quant(buf->mQuantize);
        break;
      }
      case VA::TexCoord0:
      case VA::TexCoord1:
      case VA::TexCoord2:
      case VA::TexCoord3:
      case VA::TexCoord4:
      case VA::TexCoord5:
      case VA::TexCoord6:
      case VA::TexCoord7: {
        const auto chan =
            static_cast<int>(attr) - static_cast<int>(VA::TexCoord0);

        const auto* buf = findByName(mdl.texcoords, mesh.mTexCoordBuffer[chan]);
        if (!buf) {
          return std::unexpected("Cannot find texcoord buffer " +
                                 mesh.mTexCoordBuffer[chan]);
        }
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Normal:
      case VA::NormalBinormalTangent: {
        const auto* buf = findByName(mdl.normals, mesh.mNormalBuffer);
        if (!buf) {
          return std::unexpected("Cannot find normal buffer " +
                                 mesh.mNormalBuffer);
        }
        set_quant(buf->mQuantize);
        break;
      }
      default:
        break;
      }

      std::pair<librii::gx::VertexAttribute, librii::gx::VQuantization> tmp{
          attr, quant};
      desc.push_back(tmp);
    }
    return {};
  }
};

u32 ComputeVertexSetupDLSizeFromVCD(u32 bitfield) {
  u32 texcoord0 = static_cast<u32>(librii::gx::VertexAttribute::TexCoord0);
  u32 size = 12 * 12;
  {
    u32 vcd = bitfield;
    if ((vcd & (0b111111 << (texcoord0 + 2))) == 0) {
      size = 48;
    } else if ((vcd & (0b111 << (texcoord0 + 5))) == 0) {
      size = 80;
    } else if ((vcd & (0b1 << (texcoord0 + 7))) == 0) {
      size = 112;
    } else {
      size = 144;
    }
  }
  return size;
}

struct DLSetup {
  DLCache cache{};
  VertexDescriptor desc{};

  Result<void> Build(const librii::g3d::PolygonData& mesh,
                     const librii::g3d::BinaryModel& mdl) {
    cache.descv = mesh.mVertexDescriptor;
    return desc.Build(mesh.mVertexDescriptor, mdl, mesh);
  }
  void Write(oishii::Writer& writer) {
    cache.write(writer);
    librii::gpu::DLBuilder dl(writer);
    dl.setVtxAttrFmtv(0, desc.desc);
    u32 size = ComputeVertexSetupDLSizeFromVCD(cache.descv.mBitfield);
    writer.skip(size - 1); // array pointers set on runtime
    writer.write<u8>(0);
    dl.align();
  }
};

static Result<std::vector<librii::gx::MatrixPrimitive>>
ReadMPrims(rsl::SafeReader& reader, u32 buf_size,
           const librii::gx::VertexDescriptor& desc, int currentMatrix = -1) {
  std::vector<librii::gx::MatrixPrimitive> result;
  struct QDisplayListMeshHandler final
      : public librii::gpu::QDisplayListHandler {
    Result<void> onCommandDraw(oishii::BinaryReader& reader,
                               librii::gx::PrimitiveType type, u16 nverts,
                               u32 stream_end) override {
      EXPECT(reader.tell() < stream_end);

      mLoadingMatrices = 0;
      mLoadingNrmMatrices = 0;
      mLoadingTexMatrices = 0;

      if (mPoly.mMatrixPrimitives.empty()) {
        mPoly.mMatrixPrimitives.push_back(librii::gx::MatrixPrimitive{});

        // This is a J3D-only field, but we can fill it out anyway
        mPoly.mMatrixPrimitives.back().mCurrentMatrix = mCurrentMatrix;
      }
      auto& prim = mPoly.mMatrixPrimitives.back().mPrimitives.emplace_back(
          librii::gx::IndexedPrimitive{});
      prim.mType = type;
      prim.mVertices.resize(nverts);
      for (auto& vert : prim.mVertices) {
        for (u32 i = 0; i < static_cast<u32>(librii::gx::VertexAttribute::Max);
             ++i) {
          if (mPoly.mVertexDescriptor.mBitfield & (1 << i)) {
            const auto attr = static_cast<librii::gx::VertexAttribute>(i);
            switch (mPoly.mVertexDescriptor.mAttributes[attr]) {
            case librii::gx::VertexAttributeType::Direct:
              if (attr ==
                      librii::gx::VertexAttribute::PositionNormalMatrixIndex ||
                  ((u32)attr >=
                       (u32)librii::gx::VertexAttribute::Texture0MatrixIndex &&
                   (u32)attr <=
                       (u32)librii::gx::VertexAttribute::Texture7MatrixIndex)) {
                vert[attr] =
                    TRY(reader.tryRead<u8, oishii::EndianSelect::Big, true>());
                break;
              }
              return std::unexpected("Mesh unsupported");
            case librii::gx::VertexAttributeType::None:
              break;
            case librii::gx::VertexAttributeType::Byte:
              vert[attr] =
                  TRY(reader.tryRead<u8, oishii::EndianSelect::Big, true>());
              break;
            case librii::gx::VertexAttributeType::Short:
              vert[attr] =
                  TRY(reader.tryRead<u16, oishii::EndianSelect::Big, true>());
              break;
            }
          }
        }
      }
      return {};
    }
    Result<void> onCommandIndexedLoad(u32 cmd, u32 index, u16 address,
                                      u8 size) override {
      // Expect IDX_A (position matrix) commands to be in sequence then IDX_B
      // (normal matrix) commands to follow.
      if (cmd == 0x20) {
        EXPECT(address == mLoadingMatrices * 12, "");
        EXPECT(size == 12, "");
        ++mLoadingMatrices;
      } else if (cmd == 0x28) {
        EXPECT(address == 1024 + mLoadingNrmMatrices * 9, "");
        EXPECT(size == 9, "");
        ++mLoadingNrmMatrices;
      } else if (cmd == 0x30) {
        EXPECT(address == 120 + mLoadingTexMatrices * 12, "");
        EXPECT(size == 12, "");
        ++mLoadingTexMatrices;
      } else {
        EXPECT(false, "Unknown matrix command");
      }

      // NOTE: Normal-matrix-only models would be ignored currently
      if (cmd != 0x20) {
        return {};
      }
      if (mLoadingMatrices == 1) {
        mPoly.mMatrixPrimitives.emplace_back();
      }
      auto& mp = mPoly.mMatrixPrimitives.back();

      mp.mDrawMatrixIndices.push_back(index);
      // This is a J3D-only field, but we can fill it out anyway
      mp.mCurrentMatrix = mCurrentMatrix;
      return {};
    }
    int mLoadingMatrices = 0;
    int mLoadingNrmMatrices = 0;
    int mLoadingTexMatrices = 0;
    librii::gx::MeshData mPoly;
    int mCurrentMatrix = -1;
  } meshHandler;
  meshHandler.mPoly.mVertexDescriptor = desc;
  meshHandler.mCurrentMatrix = currentMatrix;
  TRY(librii::gpu::RunDisplayList(reader.getUnsafe(), meshHandler, buf_size));
  return meshHandler.mPoly.mMatrixPrimitives;
}

static Result<librii::gx::VertexDescriptor>
BuildVCD(rsl::SafeReader& reader, u32 buf_size, u32& bitfield) {
  librii::gx::VertexDescriptor result;
  librii::gpu::QDisplayListVertexSetupHandler vcdHandler;
  TRY(librii::gpu::RunDisplayList(reader.getUnsafe(), vcdHandler, buf_size));
  for (u32 i = 0; i < (u32)librii::gx::VertexAttribute::Max; ++i) {
    // TODO: We probably don't need this bitfield
    if ((bitfield & (1 << i)) == 0)
      continue;
    auto att = static_cast<librii::gx::VertexAttribute>(i);

    if (att == librii::gx::VertexAttribute::PositionNormalMatrixIndex ||
        att == librii::gx::VertexAttribute::Texture0MatrixIndex ||
        att == librii::gx::VertexAttribute::Texture1MatrixIndex ||
        att == librii::gx::VertexAttribute::Texture2MatrixIndex) {
      result.mAttributes[att] = librii::gx::VertexAttributeType::Direct;
    } else if (i >= (u32)librii::gx::VertexAttribute::Position &&
               i <= (u32)librii::gx::VertexAttribute::TexCoord7) {
      const auto stat = vcdHandler.mGpuMesh.VCD.GetVertexArrayStatus(
          i - (u32)librii::gx::VertexAttribute::Position);
      const auto encoding = static_cast<librii::gx::VertexAttributeType>(stat);
      if (encoding == librii::gx::VertexAttributeType::None) {
        // CTools minimaps will encounter this case, so it's better for us to
        // silently ignore it than halt all progress.
        rsl::error("BuildVCD: att == librii::gx::VertexAttributeType::None is "
                   "an unexpected case from an invalid vertex coordinate "
                   "descriptor -- is this a CTools model?");
        bitfield &= ~(1 << i);
        continue;
      }
      result.mAttributes[att] = encoding;
    } else {
      return std::unexpected("Unrecognized attribute");
    }
  }
  return result;
}

struct BinaryPolygon {
  s32 currentMatrix{};
  std::array<u32, 3> vtxDescvCache{};

  u32 vcdBitfield{};
  enum {
    FLAG_CUR_MTX_INCLUDED = (1 << 0),
    FLAG_INVISIBLE = (1 << 1),
  };
  u32 flag{};
  std::string name{};
  u32 id{};
  u32 vtxCount{};
  u32 triCount{};

  s16 posIdx{};
  s16 nrmIdx{};
  std::array<s16, 2> clrIdx{};
  std::array<s16, 8> uvIdx{};
  s16 furVecIdx{};
  s16 furPosIdx{};
  std::vector<u16> used; // matrix IDs

  // NOTE: On read, only, only `.cache` is configured. `.desc` is not. Ideally
  // this won't be the case much longer.
  DLSetup dl_setup;

  std::vector<librii::gx::MatrixPrimitive> matrixPrims;

  void writeA(oishii::Writer& w) {
    w.write(currentMatrix);
    w.write(vtxDescvCache[0]);
    w.write(vtxDescvCache[1]);
    w.write(vtxDescvCache[2]);
  }
  Result<void> readA(rsl::SafeReader& r) {
    currentMatrix = TRY(r.S32());
    for (auto& u : vtxDescvCache) {
      u = TRY(r.U32());
    }
    return {};
  }
  void writeB(oishii::Writer& w, NameTable& names, u32 mesh_start) {
    w.write(vcdBitfield);
    w.write(flag);
    writeNameForward(names, w, mesh_start, name);
    w.write(id);
    w.write(vtxCount);
    w.write(triCount);
    w.write(posIdx);
    w.write(nrmIdx);
    for (size_t i = 0; i < 2; ++i)
      w.write(clrIdx[i]);
    for (size_t i = 0; i < 8; ++i)
      w.write(uvIdx[i]);
    w.write(furVecIdx);
    w.write(furPosIdx);
    w.write<s32>(0x68); // msu, directly follows
    w.write<u32>(used.size());
    if (used.size() == 0) {
      w.write<u32>(0);
    } else {
      for (const auto& matrix : used) {
        w.write<u16>(matrix);
      }
    }
  }
  Result<void> readB(rsl::SafeReader& reader, u32 start) {
    vcdBitfield = TRY(reader.U32());
    flag = TRY(reader.U32());
    name = TRY(reader.StringOfs(start));
    id = TRY(reader.U32());
    vtxCount = TRY(reader.U32());
    triCount = TRY(reader.U32());
    posIdx = TRY(reader.S16());
    nrmIdx = TRY(reader.S16());

    for (size_t i = 0; i < 2; ++i)
      clrIdx[i] = TRY(reader.S16());
    for (size_t i = 0; i < 8; ++i)
      uvIdx[i] = TRY(reader.S16());
    furVecIdx = TRY(reader.S16());
    furPosIdx = TRY(reader.S16());
    TRY(reader.S32()); // matrix usage
                       // TODO: The actual table is ignored
    return {};
  }
  // SYNC with PolygonData
  bool needsPositionMtx() const {
    // TODO: Do we need to check currentMatrixEmbedded?
    return currentMatrix == -1;
  }
  bool needsNormalMtx() const {
    // TODO: Do we need to check currentMatrixEmbedded?
    return vcdBitfield & (1 << static_cast<u32>(gx::VertexAttribute::Normal));
  }
  bool needsAnyTextureMtx() const {
    // TODO: Do we need to check currentMatrixEmbedded?
    u32 tex0idx = static_cast<u32>(gx::VertexAttribute::Texture0MatrixIndex);
    return vcdBitfield & (0b1111'1111 << tex0idx);
  }
  bool needsTextureMtx(u32 i) const {
    // TODO: Do we need to check currentMatrixEmbedded?
    assert(i < 8);
    u32 tex0idx = static_cast<u32>(gx::VertexAttribute::Texture0MatrixIndex);
    return vcdBitfield & (1 << (tex0idx + i));
  }

  Result<void>
  writeVertexDataDL(const librii::gx::MatrixPrimitive& mp,
                    const librii::gx::VertexDescriptor& mVertexDescriptor,
                    oishii::Writer& writer) {
    using VATAttrib = librii::gx::VertexAttribute;
    using VATType = librii::gx::VertexAttributeType;

    if (currentMatrix == -1) {
      librii::gpu::DLBuilder builder(writer);
      bool anyNeedsTextureMtx = false;
      for (u32 i = 0; i < 8; ++i) {
        if (needsTextureMtx(i)) {
          anyNeedsTextureMtx = true;
          break;
        }
      }
      // Only one copy needed!
      if (anyNeedsTextureMtx) {
        for (size_t i = 0; i < mp.mDrawMatrixIndices.size(); ++i) {
          if (mp.mDrawMatrixIndices[i] == -1) {
            continue;
          }
          builder.loadTexMtxIndx(mp.mDrawMatrixIndices[i], 30 + i * 3,
                                 librii::gx::TexGenType::Matrix3x4);
        }
      }
      if (needsPositionMtx()) {
        for (size_t i = 0; i < mp.mDrawMatrixIndices.size(); ++i) {
          if (mp.mDrawMatrixIndices[i] == -1) {
            continue;
          }
          builder.loadPosMtxIndx(mp.mDrawMatrixIndices[i], i * 3);
        }
      }
      if (needsNormalMtx()) {
        for (size_t i = 0; i < mp.mDrawMatrixIndices.size(); ++i) {
          if (mp.mDrawMatrixIndices[i] == -1) {
            continue;
          }
          builder.loadNrmMtxIndx3x3(mp.mDrawMatrixIndices[i], i * 3);
        }
      }
    }

    for (auto& prim : mp.mPrimitives) {
      u8 prim_cmd = librii::gx::EncodeDrawPrimitiveCommand(prim.mType);
      if (prim.mVertices.size() > 0xFFFF) {
        return std::unexpected(
            "Too many vertices in single primitive (max 0xFFFF)");
      }
      u16 indices_count = prim.mVertices.size();

      writer.writeUnaligned<u8>(prim_cmd);
      writer.writeUnaligned<u16>(indices_count);
      for (const auto& v : prim.mVertices) {
        for (int a = 0; a < (int)VATAttrib::Max; ++a) {
          VATAttrib attr = static_cast<VATAttrib>(a);
          if ((vcdBitfield & (1 << a)) == 0)
            continue;
          switch (mVertexDescriptor.mAttributes.at(attr)) {
          case VATType::None:
            break;
          case VATType::Byte:
            writer.writeUnaligned<u8>(v[attr]);
            break;
          case VATType::Short:
            writer.writeUnaligned<u16>(v[attr]);
            break;
          case VATType::Direct:
            if (attr != VATAttrib::PositionNormalMatrixIndex &&
                ((u32)attr > (u32)VATAttrib::Texture7MatrixIndex &&
                 (u32)attr < (u32)VATAttrib::Texture0MatrixIndex)) {
              return std::unexpected("Direct vertex data is unsupported.");
            }
            writer.writeUnaligned<u8>(v[attr]);
            break;
          default:
            return std::unexpected("Unknown vertex attribute format.");
          }
        }
      }
    }

    return {};
  }

  Result<void> write(oishii::Writer& writer, NameTable& names, u32 mesh_start) {
    // Write first part of header
    writeA(writer);
    // Write placeholders for DL pts
    DlHandle setup(writer);
    DlHandle data(writer);
    // Write end of header
    writeB(writer, names, mesh_start);

    // Write DLSetup
    {
      writer.alignTo(32);
      auto setup_ba = writer.tell();
      setup.setBufAddr(writer.tell());
      { //
        dl_setup.Write(writer);
      }
      setup.setCmdSize(writer.tell() - setup_ba);
      while (writer.tell() != setup_ba + 0xe0)
        writer.write<u8>(0);
      setup.setBufSize(0xe0);
      setup.write();
    }

    // Write VertexDataDL
    {
      writer.alignTo(32);
      data.setBufAddr(writer.tell());
      const auto data_start = writer.tell();
      {
        for (auto& mp : matrixPrims) {
          TRY(writeVertexDataDL(mp, dl_setup.cache.descv, writer));
        }
        // DL pad
        while (writer.tell() % 32) {
          writer.write<u8>(0);
        }
      }
      data.setCmdSize(writer.tell() - data_start);
      writer.alignTo(32);
      data.write();
    }
    return {};
  }

  Result<void> read(rsl::SafeReader& reader, u32 start) {
    // Read-only
    TRY(readA(reader));
    // TODO: Check cache

    DlHandle primitiveSetup = TRY(DlHandle::make(reader));
    DlHandle primitiveData = TRY(DlHandle::make(reader));

    TRY(readB(reader, start));

    // Read primitiveSetup
    primitiveSetup.seekTo(reader);
    // vcdBitfield may be mutated to accomodate CTools models having ::None
    // attributes specified..
    auto desc = TRY(BuildVCD(reader, primitiveSetup.buf_size, vcdBitfield));
    desc.calcVertexDescriptorFromAttributeList();
    EXPECT(desc.mBitfield == vcdBitfield);
    dl_setup.cache.descv = desc;
    // TODO: dl_setup is not properly setup for save directly

    // Read primitiveData
    primitiveData.seekTo(reader);
    matrixPrims =
        TRY(ReadMPrims(reader, primitiveData.buf_size, desc, currentMatrix));

    return {};
  }
};

std::optional<u8> asTexNMtx(gx::VertexAttribute attr) {
  u32 u = static_cast<u32>(attr);
  if (u >= static_cast<u32>(gx::VertexAttribute::Texture0MatrixIndex) &&
      u <= static_cast<u32>(gx::VertexAttribute::Texture7MatrixIndex)) {
    return u - static_cast<u32>(gx::VertexAttribute::Texture0MatrixIndex);
  }
  return std::nullopt;
}

Result<void>
ReadMesh(librii::g3d::PolygonData& poly, rsl::SafeReader& reader, bool& isValid,

         const std::vector<librii::g3d::PositionBuffer>& positions,
         const std::vector<librii::g3d::NormalBuffer>& normals,
         const std::vector<librii::g3d::ColorBuffer>& colors,
         const std::vector<librii::g3d::TextureCoordinateBuffer>& texcoords,

         kpi::LightIOTransaction& transaction,
         const std::string& transaction_path,

         u32 id, bool* badfur) {
  const auto start = reader.tell();

  isValid &= TRY(reader.U32()) != 0; // size
  isValid &= TRY(reader.S32()) < 0;  // mdl offset

  BinaryPolygon bin;
  TRY(bin.read(reader, start));

  EXPECT((bin.flag & BinaryPolygon::FLAG_CUR_MTX_INCLUDED) == false);
#if 0
  EXPECT((bin.flag & BinaryPolygon::FLAG_CUR_MTX_INCLUDED) ==
         (bin.dl_setup.cache
              .descv[gx::VertexAttribute::PositionNormalMatrixIndex] &&
          bin.matrixPrims.size() > 1));
#endif

  // Take out TexNMtxIdx
  //
  // Right now, we don't check and simply purge the data, trusting that it will
  // be added back properly.
  auto& desc = bin.dl_setup.cache.descv;
  std::bitset<8> texmtx_specified;
  for (auto& attr : desc.mAttributes) {
    auto texn = asTexNMtx(attr.first);
    if (texn.has_value()) {
      EXPECT(*texn < texmtx_specified.size());
      texmtx_specified[*texn] = true;
    }
  }
  std::erase_if(desc.mAttributes, [](auto& attr) -> bool {
    auto t = asTexNMtx(attr.first);
    return t.has_value();
  });
  desc.calcVertexDescriptorFromAttributeList();
  // Because of the way IndexedVertex works, we don't have to touch the actual
  // vertex arrays

  poly.mCurrentMatrix = bin.currentMatrix;
  poly.mVertexDescriptor = bin.dl_setup.cache.descv;
  poly.visible = !(bin.flag & BinaryPolygon::FLAG_INVISIBLE);
  poly.mName = bin.name;
  EXPECT(bin.id == id);
  // TODO: Verify / cache
  isValid &= bin.vtxCount > 0; // nVert
  isValid &= bin.triCount > 0; // nPoly
  if (bin.posIdx >= 0) {
    poly.mPositionBuffer = positions[bin.posIdx].mName;
  }
  if (bin.nrmIdx >= 0) {
    poly.mNormalBuffer = normals[bin.nrmIdx].mName;
  }
  for (size_t i = 0; i < 2; ++i) {
    if (bin.clrIdx[i] >= 0) {
      poly.mColorBuffer[i] = colors[bin.clrIdx[i]].mName;
    }
  }
  for (size_t i = 0; i < 8; ++i) {
    if (bin.uvIdx[i] >= 0) {
      poly.mTexCoordBuffer[i] = texcoords[bin.uvIdx[i]].mName;
    }
  }
  // BrawlCrate sets this erroneously
  if (bin.furVecIdx != -1 || bin.furPosIdx != -1) {
    if (badfur != nullptr)
      *badfur = true;
  }
  poly.mMatrixPrimitives = bin.matrixPrims;
  return {};
}

Result<BinaryPolygon> toBinPoly(const librii::g3d::PolygonData& mesh,
                                const librii::g3d::BinaryModel& mdl, u32 id,
                                std::bitset<8> texmtx_needed) {
  BinaryPolygon bin;
  bin.currentMatrix = mesh.mCurrentMatrix;
  bin.flag = 0;
#if 0
  if (mesh.mVertexDescriptor[gx::VertexAttribute::PositionNormalMatrixIndex] &&
      bin.matrixPrims.size() > 1) {
    bin.flag |= BinaryPolygon::FLAG_CUR_MTX_INCLUDED;
  }
#endif
  if (!mesh.visible) {
    bin.flag |= BinaryPolygon::FLAG_INVISIBLE;
  }
  bin.name = mesh.getName();
  bin.id = id;
  const auto [nvtx, ntri] = librii::gx::ComputeVertTriCounts(mesh);
  bin.vtxCount = nvtx;
  bin.triCount = ntri;

  bin.posIdx = indexOf(mdl.positions, mesh.mPositionBuffer);
  bin.nrmIdx = indexOf(mdl.normals, mesh.mNormalBuffer);
  for (size_t i = 0; i < 2; ++i)
    bin.clrIdx[i] = indexOf(mdl.colors, mesh.mColorBuffer[i]);
  for (size_t i = 0; i < 8; ++i)
    bin.uvIdx[i] = indexOf(mdl.texcoords, mesh.mTexCoordBuffer[i]);
  bin.furVecIdx = -1;
  bin.furPosIdx = -1;

  std::set<u16> used;
  for (auto& mp : mesh.mMatrixPrimitives) {
    for (auto& w : mp.mDrawMatrixIndices) {
      if (w < 0) {
        continue;
      }
      used.insert(static_cast<u16>(w));
    }
  }
  bin.used = {used.begin(), used.end()};

  // Add TEXNMTX to VCD
  auto tmp = mesh;
  if (tmp.mVertexDescriptor.mAttributes.contains(
          gx::VertexAttribute::PositionNormalMatrixIndex)) {
    for (size_t i = 0; i < 8; ++i) {
      bool needed = texmtx_needed[i];
      if (!needed)
        continue;
      librii::gx::VertexAttribute f = static_cast<librii::gx::VertexAttribute>(
          static_cast<int>(librii::gx::VertexAttribute::Texture0MatrixIndex) +
          i);
      tmp.mVertexDescriptor.mAttributes[f] = gx::VertexAttributeType::Direct;
      tmp.mVertexDescriptor.mBitfield |= 2 << i;
    }
  }
  auto& vcd = tmp.mVertexDescriptor;
  vcd.calcVertexDescriptorFromAttributeList();
  bin.vcdBitfield = vcd.mBitfield;

  const auto [c_a, c_b, c_c] = librii::gpu::DLBuilder::calcVtxDescv(vcd);
  bin.vtxDescvCache[0] = c_a;
  bin.vtxDescvCache[1] = c_b;
  bin.vtxDescvCache[2] = c_c;

  TRY(bin.dl_setup.Build(tmp, mdl));

  bin.matrixPrims = tmp.mMatrixPrimitives;

  // Add TEXNMTX to VERTEX DATA
  if (tmp.mVertexDescriptor[gx::VertexAttribute::PositionNormalMatrixIndex]) {
    for (size_t i = 0; i < 8; ++i) {
      bool needed = texmtx_needed[i];
      if (!needed)
        continue;
      librii::gx::VertexAttribute f = static_cast<librii::gx::VertexAttribute>(
          static_cast<int>(librii::gx::VertexAttribute::Texture0MatrixIndex) +
          i);
      for (auto& mp : bin.matrixPrims) {
        for (auto& p : mp.mPrimitives) {
          for (auto& v : p.mVertices) {
            v[f] = v[gx::VertexAttribute::PositionNormalMatrixIndex] + 30;
          }
        }
      }
    }
  }

  return bin;
}

Result<void> WriteMesh(oishii::Writer& writer,
                       const librii::g3d::PolygonData& mesh,
                       const librii::g3d::BinaryModel& mdl,
                       const size_t& mesh_start, NameTable& names, u32 id,
                       std::bitset<8> texmtx_needed) {
  auto bin = TRY(toBinPoly(mesh, mdl, id, texmtx_needed));
  return bin.write(writer, names, mesh_start);
}

} // namespace librii::g3d
