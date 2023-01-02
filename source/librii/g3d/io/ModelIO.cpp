#include "ModelIO.hpp"

#include "CommonIO.hpp"
// XXX: Must not include DictIO.hpp when using DictWriteIO.hpp
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/ModelIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLInterpreter.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>

///// Headers of glm_io.hpp
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vendor/glm/vec2.hpp>
#include <vendor/glm/vec3.hpp>

auto indexOf = [](const auto& x, const auto& y) -> int {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? -1 : index;
};
auto findByName = [](const auto& x, const auto& y) {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? nullptr : &x[index];
};

namespace librii::g3d {

template <bool Named, bool bMaterial, typename T, typename U>
void writeDictionary(const std::string& name, T&& src_range, U handler,
                     RelocWriter& linker, oishii::Writer& writer, u32 mdl_start,
                     NameTable& names, int* d_cursor, bool raw = false,
                     u32 align = 4, bool BetterMethod = false) {
  if (src_range.size() == 0)
    return;

  u32 dict_ofs;
  librii::g3d::BetterDictionary _dict;
  _dict.nodes.resize(src_range.size());

  if (BetterMethod) {
    dict_ofs = writer.tell();
  } else {
    dict_ofs = *d_cursor;
    *d_cursor += CalcDictionarySize(_dict);
  }

  if (BetterMethod)
    writer.skip(CalcDictionarySize(_dict));

  for (std::size_t i = 0; i < src_range.size(); ++i) {
    writer.alignTo(align); //
    _dict.nodes[i].stream_pos = writer.tell();
    if constexpr (Named) {
      if constexpr (bMaterial)
        _dict.nodes[i].name = src_range[i].name;
      else
        _dict.nodes[i].name = src_range[i].getName();
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
    WriteDictionary(_dict, writer, names);
  }
}

///// Bring body of glm_io.hpp into namespace (without headers)
#include <core/util/glm_io.hpp>

template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
Result<void> readGenericBuffer(
    librii::g3d::GenericBuffer<T, HasMinimum, HasDivisor, kind>& out,
    rsl::SafeReader& reader) {
  const auto start = reader.tell();
  out.mEntries.clear();

  TRY(reader.U32()); // size
  TRY(reader.U32()); // mdl0 offset
  const auto startOfs = TRY(reader.S32());
  out.mName = TRY(reader.StringOfs(start));
  out.mId = TRY(reader.U32());
  out.mQuantize.mComp = librii::gx::VertexComponentCount(
      static_cast<librii::gx::VertexComponentCount::Normal>(TRY(reader.U32())));
  out.mQuantize.mType = librii::gx::VertexBufferType(
      static_cast<librii::gx::VertexBufferType::Color>(TRY(reader.U32())));
  if (HasDivisor) {
    out.mQuantize.divisor = TRY(reader.U8());
    out.mQuantize.stride = TRY(reader.U8());
  } else {
    out.mQuantize.divisor = 0;
    out.mQuantize.stride = TRY(reader.U8());
    TRY(reader.U8());
  }
  auto err = ValidateQuantize(kind, out.mQuantize);
  if (!err.empty()) {
    return std::unexpected(err);
  }
  out.mEntries.resize(TRY(reader.U16()));
  if constexpr (HasMinimum) {
    T minEnt, maxEnt;
    minEnt << reader.getUnsafe();
    maxEnt << reader.getUnsafe();

    out.mCachedMinMax = MinMax<T>{.min = minEnt, .max = maxEnt};
  }
  const auto nComponents =
      TRY(librii::gx::computeComponentCount(kind, out.mQuantize.mComp));

  reader.seekSet(start + startOfs);
  for (auto& entry : out.mEntries) {
    entry = TRY(librii::gx::readComponents<T>(reader.getUnsafe(),
                                              out.mQuantize.mType, nComponents,
                                              out.mQuantize.divisor));
  }

  if constexpr (HasMinimum) {
    // Unset if not needed
    if (out.mCachedMinMax.has_value() &&
        librii::g3d::ComputeMinMax(out) == *out.mCachedMinMax) {
      out.mCachedMinMax = std::nullopt;
    }
  }

  return {};
}

// TODO: Move to own files

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

Result<void>
ReadMesh(librii::g3d::PolygonData& poly, rsl::SafeReader& reader, bool& isValid,

         const std::vector<librii::g3d::PositionBuffer>& positions,
         const std::vector<librii::g3d::NormalBuffer>& normals,
         const std::vector<librii::g3d::ColorBuffer>& colors,
         const std::vector<librii::g3d::TextureCoordinateBuffer>& texcoords,

         kpi::LightIOTransaction& transaction,
         const std::string& transaction_path) {
  const auto start = reader.tell();

  isValid &= TRY(reader.U32()) != 0; // size
  isValid &= TRY(reader.S32()) < 0;  // mdl offset

  poly.mCurrentMatrix = TRY(reader.S32());
  reader.getUnsafe().skip(12); // cache

  DlHandle primitiveSetup = TRY(DlHandle::make(reader));
  DlHandle primitiveData = TRY(DlHandle::make(reader));

  poly.mVertexDescriptor.mBitfield = TRY(reader.U32());
  const u32 flag = TRY(reader.U32());
  poly.currentMatrixEmbedded = flag & 1;
  if (poly.currentMatrixEmbedded) {
    // TODO (should be caught later)
  }
  poly.visible = !(flag & 2);

  poly.mName = TRY(reader.StringOfs(start));
  poly.mId = TRY(reader.U32());
  // TODO: Verify / cache
  isValid &= TRY(reader.U32()) > 0; // nVert
  isValid &= TRY(reader.U32()) > 0; // nPoly

  auto readBufHandle = [&](auto ifExist) -> Result<std::string> {
    const auto hid = TRY(reader.S16());
    if (hid < 0)
      return "";
    return ifExist(hid);
  };

  poly.mPositionBuffer =
      TRY(readBufHandle([&](s16 hid) { return positions[hid].mName; }));
  poly.mNormalBuffer =
      TRY(readBufHandle([&](s16 hid) { return normals[hid].mName; }));
  for (int i = 0; i < 2; ++i) {
    poly.mColorBuffer[i] =
        TRY(readBufHandle([&](s16 hid) { return colors[hid].mName; }));
  }
  for (int i = 0; i < 8; ++i) {
    poly.mTexCoordBuffer[i] =
        TRY(readBufHandle([&](s16 hid) { return texcoords[hid].mName; }));
  }
  isValid &= TRY(reader.S32()) == -1; // fur
  TRY(reader.S32());                  // matrix usage

  primitiveSetup.seekTo(reader);
  librii::gpu::QDisplayListVertexSetupHandler vcdHandler;
  TRY(librii::gpu::RunDisplayList(reader.getUnsafe(), vcdHandler,
                                  primitiveSetup.buf_size));

  for (u32 i = 0; i < (u32)librii::gx::VertexAttribute::Max; ++i) {
    if ((poly.mVertexDescriptor.mBitfield & (1 << i)) == 0)
      continue;
    auto att = static_cast<librii::gx::VertexAttribute>(i);

    if (att == librii::gx::VertexAttribute::PositionNormalMatrixIndex ||
        att == librii::gx::VertexAttribute::Texture0MatrixIndex ||
        att == librii::gx::VertexAttribute::Texture1MatrixIndex ||
        att == librii::gx::VertexAttribute::Texture2MatrixIndex) {
      poly.mVertexDescriptor.mAttributes[att] =
          librii::gx::VertexAttributeType::Direct;
    } else if (i >= (u32)librii::gx::VertexAttribute::Position &&
               i <= (u32)librii::gx::VertexAttribute::TexCoord7) {
      const auto stat = vcdHandler.mGpuMesh.VCD.GetVertexArrayStatus(
          i - (u32)librii::gx::VertexAttribute::Position);
      const auto encoding = static_cast<librii::gx::VertexAttributeType>(stat);
      if (encoding == librii::gx::VertexAttributeType::None) {
        transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                             "att == librii::gx::VertexAttributeType::None");
        poly.mVertexDescriptor.mBitfield ^= (1 << i);
        // transaction.state = kpi::TransactionState::Failure;
        // return;
      }
      poly.mVertexDescriptor.mAttributes[att] = encoding;
    } else {
      EXPECT(false, "Unrecognized attribute");
    }
  }
  struct QDisplayListMeshHandler final
      : public librii::gpu::QDisplayListHandler {
    Result<void> onCommandDraw(oishii::BinaryReader& reader,
                               librii::gx::PrimitiveType type, u16 nverts,
                               u32 stream_end) override {
      if (mErr)
        return {};

      EXPECT(reader.tell() < stream_end);

      mLoadingMatrices = 0;
      mLoadingNrmMatrices = 0;
      mLoadingTexMatrices = 0;

      if (mPoly.mMatrixPrimitives.empty()) {
        mPoly.mMatrixPrimitives.push_back(librii::gx::MatrixPrimitive{});

        // This is a J3D-only field, but we can fill it out anyway
        mPoly.mMatrixPrimitives.back().mCurrentMatrix = mPoly.mCurrentMatrix;
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
                vert[attr] = reader.readUnaligned<u8>();
                break;
              }
              mErr = true;
              return {};
            case librii::gx::VertexAttributeType::None:
              break;
            case librii::gx::VertexAttributeType::Byte:
              vert[attr] = reader.readUnaligned<u8>();
              break;
            case librii::gx::VertexAttributeType::Short:
              vert[attr] = reader.readUnaligned<u16>();
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
      mp.mCurrentMatrix = mPoly.mCurrentMatrix;
      return {};
    }
    QDisplayListMeshHandler(librii::g3d::PolygonData& poly) : mPoly(poly) {}
    bool mErr = false;
    int mLoadingMatrices = 0;
    int mLoadingNrmMatrices = 0;
    int mLoadingTexMatrices = 0;
    librii::g3d::PolygonData& mPoly;
  } meshHandler(poly);
  primitiveData.seekTo(reader);
  TRY(librii::gpu::RunDisplayList(reader.getUnsafe(), meshHandler,
                                  primitiveData.buf_size));
  if (meshHandler.mErr) {
    transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                         "Mesh unsupported.");
    transaction.state = kpi::TransactionState::Failure;
    return {};
  }
  return {};
}

Result<void> BinaryModel::read(oishii::BinaryReader& unsafeReader,
                               kpi::LightIOTransaction& transaction,
                               const std::string& transaction_path,
                               bool& isValid) {
  rsl::SafeReader reader(unsafeReader);
  const auto start = reader.tell();
  TRY(reader.Magic("MDL0"));
  MAYBE_UNUSED const u32 fileSize = TRY(reader.U32());
  const u32 revision = TRY(reader.U32());
  if (revision != 11) {
    return std::unexpected(std::format(
        "MDL0 is version {}. Only MDL0 version 11 is supported.", revision));
  }

  TRY(reader.S32()); // ofsBRRES

  union {
    struct {
      s32 ofsRenderTree;
      s32 ofsBones;
      struct {
        s32 position;
        s32 normal;
        s32 color;
        s32 uv;
        s32 furVec;
        s32 furPos;
      } ofsBuffers;
      s32 ofsMaterials;
      s32 ofsShaders;
      s32 ofsMeshes;
      s32 ofsTextureLinks;
      s32 ofsPaletteLinks;
      s32 ofsUserData;
    } secOfs;
    std::array<s32, 14> secOfsArr;
  };
  for (auto& ofs : secOfsArr)
    ofs = TRY(reader.S32());

  name = TRY(reader.StringOfs(start));
  info.read(reader);

  auto readDict = [&](u32 xofs, auto handler) -> Result<void> {
    if (!xofs) {
      return {};
    }
    reader.seekSet(start + xofs);
    auto dict = TRY(ReadDictionary(reader));
    for (auto& node : dict.nodes) {
      EXPECT(node.stream_pos);
      reader.seekSet(node.stream_pos);
      TRY(handler(node));
    }
    return {};
  };

  TRY(readDict(secOfs.ofsBones, [&](const librii::g3d::BetterNode& dnode) {
    auto& bone = bones.emplace_back();
    return bone.read(unsafeReader);
  }));

  // Read Vertex data
  TRY(readDict(secOfs.ofsBuffers.position,
               [&](const librii::g3d::BetterNode& dnode) {
                 return readGenericBuffer(positions.emplace_back(), reader);
               }));
  TRY(readDict(secOfs.ofsBuffers.normal,
               [&](const librii::g3d::BetterNode& dnode) {
                 return readGenericBuffer(normals.emplace_back(), reader);
               }));
  TRY(readDict(secOfs.ofsBuffers.color,
               [&](const librii::g3d::BetterNode& dnode) {
                 return readGenericBuffer(colors.emplace_back(), reader);
               }));
  TRY(readDict(secOfs.ofsBuffers.uv, [&](const librii::g3d::BetterNode& dnode) {
    return readGenericBuffer(texcoords.emplace_back(), reader);
  }));

  if (transaction.state == kpi::TransactionState::Failure)
    return {};

  // TODO: Fur

  TRY(readDict(secOfs.ofsMaterials, [&](const librii::g3d::BetterNode& dnode) {
    auto& mat = materials.emplace_back();
    return mat.read(unsafeReader, nullptr);
  }));
  TRY(readDict(secOfs.ofsMeshes, [&](const librii::g3d::BetterNode& dnode) {
    auto& poly = meshes.emplace_back();
    return ReadMesh(poly, reader, isValid, positions, normals, colors,
                    texcoords, transaction, transaction_path);
  }));

  if (transaction.state == kpi::TransactionState::Failure)
    return {};

  TRY(readDict(secOfs.ofsRenderTree,
               [&](const librii::g3d::BetterNode& dnode) -> Result<void> {
                 auto commands = TRY(ByteCodeLists::ParseStream(unsafeReader));
                 ByteCodeMethod c{dnode.name, commands};
                 bytecodes.emplace_back(c);
                 return {};
               }));

  if (!isValid) {
    transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                         "Note: BRRES file was saved with BrawlBox. Certain "
                         "materials may flicker during ghost replays.");
  }
  if (bones.size() > 1) {
    transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                         "Rigging support is not fully tested.");
  }
  return {};
}

//
// WRITING CODE
//

namespace {
// Does not write size or mdl0 offset
template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
void writeGenericBuffer(
    const librii::g3d::GenericBuffer<T, HasMinimum, HasDivisor, kind>& buf,
    oishii::Writer& writer, u32 header_start, NameTable& names) {
  const auto backpatch_array_ofs = writePlaceholder(writer);
  writeNameForward(names, writer, header_start, buf.mName);
  writer.write<u32>(buf.mId);
  writer.write<u32>(static_cast<u32>(buf.mQuantize.mComp.position));
  writer.write<u32>(static_cast<u32>(buf.mQuantize.mType.generic));
  if constexpr (HasDivisor) {
    writer.write<u8>(buf.mQuantize.divisor);
    writer.write<u8>(buf.mQuantize.stride);
  } else {
    writer.write<u8>(buf.mQuantize.stride);
    writer.write<u8>(0);
  }
  writer.write<u16>(buf.mEntries.size());
  // TODO: min/max
  if constexpr (HasMinimum) {
    auto mm = librii::g3d::ComputeMinMax(buf);

    // Necessary for matching as it is not re-quantized in original files.
    if (buf.mCachedMinMax.has_value()) {
      mm = *buf.mCachedMinMax;
    }

    mm.min >> writer;
    mm.max >> writer;
  }

  writer.alignTo(32);
  writeOffsetBackpatch(writer, backpatch_array_ofs, header_start);

  const auto nComponents =
      librii::gx::computeComponentCount(kind, buf.mQuantize.mComp);
  assert(nComponents.has_value());

  for (auto& entry : buf.mEntries) {
    librii::gx::writeComponents(writer, entry, buf.mQuantize.mType,
                                *nComponents, buf.mQuantize.divisor);
  }
  writer.alignTo(32);
}

std::string writeVertexDataDL(const librii::g3d::PolygonData& poly,
                              const librii::gx::MatrixPrimitive& mp,
                              oishii::Writer& writer) {
  using VATAttrib = librii::gx::VertexAttribute;
  using VATType = librii::gx::VertexAttributeType;

  if (poly.mCurrentMatrix == -1) {
    librii::gpu::DLBuilder builder(writer);
    if (poly.needsTextureMtx()) {
      for (size_t i = 0; i < mp.mDrawMatrixIndices.size(); ++i) {
        if (mp.mDrawMatrixIndices[i] == -1) {
          continue;
        }
        // TODO: Get this from the material?
        builder.loadTexMtxIndx(mp.mDrawMatrixIndices[i], i * 3,
                               librii::gx::TexGenType::Matrix3x4);
      }
    }
    if (poly.needsPositionMtx()) {
      for (size_t i = 0; i < mp.mDrawMatrixIndices.size(); ++i) {
        if (mp.mDrawMatrixIndices[i] == -1) {
          continue;
        }
        builder.loadPosMtxIndx(mp.mDrawMatrixIndices[i], i * 3);
      }
    }
    if (poly.needsNormalMtx()) {
      for (size_t i = 0; i < mp.mDrawMatrixIndices.size(); ++i) {
        if (mp.mDrawMatrixIndices[i] == -1) {
          continue;
        }
        builder.loadNrmMtxIndx3x3(mp.mDrawMatrixIndices[i], i * 3);
      }
    }
  }

  for (auto& prim : mp.mPrimitives) {
    writer.writeUnaligned<u8>(
        librii::gx::EncodeDrawPrimitiveCommand(prim.mType));
    writer.writeUnaligned<u16>(prim.mVertices.size());
    for (const auto& v : prim.mVertices) {
      for (int a = 0; a < (int)VATAttrib::Max; ++a) {
        VATAttrib attr = static_cast<VATAttrib>(a);
        if (!poly.mVertexDescriptor[attr])
          continue;
        switch (poly.mVertexDescriptor.mAttributes.at(attr)) {
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
            return "Direct vertex data is unsupported.";
          }
          writer.writeUnaligned<u8>(v[attr]);
          break;
        default:
          return "!Unknown vertex attribute format.";
        }
      }
    }
  }

  return "";
}

void WriteShader(RelocWriter& linker, oishii::Writer& writer,
                 const BinaryTev& tev, std::size_t shader_start,
                 int shader_id) {
  DebugReport("Shader at %x\n", (unsigned)shader_start);
  linker.label("Shader" + std::to_string(shader_id), shader_start);

  tev.writeBody(writer);
}

std::string WriteMesh(oishii::Writer& writer,
                      const librii::g3d::PolygonData& mesh,
                      const librii::g3d::BinaryModel& mdl,
                      const size_t& mesh_start, NameTable& names) {
  const auto build_dlcache = [&]() {
    librii::gpu::DLBuilder dl(writer);
    writer.skip(5 * 2); // runtime
    dl.setVtxDescv(mesh.mVertexDescriptor);
    writer.write<u8>(0);
    // dl.align(); // 1
  };
  const auto build_dlsetup = [&]() -> std::string {
    build_dlcache();
    librii::gpu::DLBuilder dl(writer);

    // Build desc
    // ----
    std::vector<
        std::pair<librii::gx::VertexAttribute, librii::gx::VQuantization>>
        desc;
    for (auto [attr, type] : mesh.mVertexDescriptor.mAttributes) {
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
          return "Cannot find position buffer " + mesh.mPositionBuffer;
        }
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color0: {
        const auto* buf = findByName(mdl.colors, mesh.mColorBuffer[0]);
        if (!buf) {
          return "Cannot find color buffer " + mesh.mColorBuffer[0];
        }
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color1: {
        const auto* buf = findByName(mdl.colors, mesh.mColorBuffer[1]);
        if (!buf) {
          return "Cannot find color buffer " + mesh.mColorBuffer[1];
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
          return "Cannot find texcoord buffer " + mesh.mTexCoordBuffer[chan];
        }
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Normal:
      case VA::NormalBinormalTangent: {
        const auto* buf = findByName(mdl.normals, mesh.mNormalBuffer);
        if (!buf) {
          return "Cannot find normal buffer " + mesh.mNormalBuffer;
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
    // ---

    dl.setVtxAttrFmtv(0, desc);
    u32 texcoord0 = static_cast<u32>(librii::gx::VertexAttribute::TexCoord0);
    u32 size = 12 * 12;
    {
      u32 vcd = mesh.mVertexDescriptor.mBitfield;
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
    writer.skip(size - 1); // array pointers set on runtime
    writer.write<u8>(0);
    dl.align();
    return "";
  };
  const auto assert_since = [&](u32 ofs) {
    MAYBE_UNUSED const auto since = writer.tell() - mesh_start;
    assert(since == ofs);
  };

  assert_since(8);
  writer.write<s32>(mesh.mCurrentMatrix);

  assert_since(12);
  const auto [c_a, c_b, c_c] =
      librii::gpu::DLBuilder::calcVtxDescv(mesh.mVertexDescriptor);
  writer.write<u32>(c_a);
  writer.write<u32>(c_b);
  writer.write<u32>(c_c);

  assert_since(24);
  DlHandle setup(writer);
  DlHandle data(writer);

  auto vcd = mesh.mVertexDescriptor;
  vcd.calcVertexDescriptorFromAttributeList();
  assert_since(0x30);
  writer.write<u32>(vcd.mBitfield);
  writer.write<u32>(static_cast<u32>(mesh.currentMatrixEmbedded) +
                    (static_cast<u32>(!mesh.visible) << 1));
  writeNameForward(names, writer, mesh_start, mesh.getName());
  writer.write<u32>(mesh.mId);
  // TODO
  assert_since(0x40);
  const auto [nvtx, ntri] = librii::gx::ComputeVertTriCounts(mesh);
  writer.write<u32>(nvtx);
  writer.write<u32>(ntri);

  assert_since(0x48);

  const auto pos_idx = indexOf(mdl.positions, mesh.mPositionBuffer);
  writer.write<s16>(pos_idx);
  writer.write<s16>(indexOf(mdl.normals, mesh.mNormalBuffer));
  for (int i = 0; i < 2; ++i)
    writer.write<s16>(indexOf(mdl.colors, mesh.mColorBuffer[i]));
  for (int i = 0; i < 8; ++i)
    writer.write<s16>(indexOf(mdl.texcoords, mesh.mTexCoordBuffer[i]));
  writer.write<s16>(-1); // fur
  writer.write<s16>(-1); // fur
  assert_since(0x64);
  writer.write<s32>(0x68); // msu, directly follows

  std::set<s16> used;
  for (auto& mp : mesh.mMatrixPrimitives) {
    for (auto& w : mp.mDrawMatrixIndices) {
      if (w == -1) {
        continue;
      }
      used.insert(w);
    }
  }
  writer.write<u32>(used.size());
  if (used.size() == 0) {
    writer.write<u32>(0);
  } else {
    for (const auto& matrix : used) {
      writer.write<u16>(matrix);
    }
  }

  writer.alignTo(32);
  auto setup_ba = writer.tell();
  setup.setBufAddr(writer.tell());
  auto dlerror = build_dlsetup();
  if (!dlerror.empty()) {
    return dlerror;
  }
  setup.setCmdSize(writer.tell() - setup_ba);
  while (writer.tell() != setup_ba + 0xe0)
    writer.write<u8>(0);
  setup.setBufSize(0xe0);
  setup.write();
  writer.alignTo(32);
  data.setBufAddr(writer.tell());
  const auto data_start = writer.tell();
  {
    for (auto& mp : mesh.mMatrixPrimitives) {
      auto err = writeVertexDataDL(mesh, mp, writer);
      if (!err.empty()) {
        return err;
      }
    }
    // DL pad
    while (writer.tell() % 32) {
      writer.write<u8>(0);
    }
  }
  data.setCmdSize(writer.tell() - data_start);
  writer.alignTo(32);
  data.write();
  return "";
}

void writeModel(librii::g3d::BinaryModel& bin, oishii::Writer& writer,
                NameTable& names, std::size_t brres_start) {
  const auto mdl_start = writer.tell();
  int d_cursor = 0;

  RelocWriter linker(writer);
  //
  // Build shaders
  //

  std::map<std::string, u32> Dictionaries;
  const auto write_dict = [&](const std::string& name, auto src_range,
                              auto handler, bool raw = false, u32 align = 4) {
    if (src_range.end() == src_range.begin())
      return;
    int d_pos = Dictionaries.at(name);
    writeDictionary<true, false>(name, src_range, handler, linker, writer,
                                 mdl_start, names, &d_pos, raw, align);
  };
  const auto write_dict_mat = [&](const std::string& name, auto& src_range,
                                  auto handler, bool raw = false,
                                  u32 align = 4) {
    if (src_range.end() == src_range.begin())
      return;
    int d_pos = Dictionaries.at(name);
    writeDictionary<true, true>(name, src_range, handler, linker, writer,
                                mdl_start, names, &d_pos, raw, align);
  };

  {
    linker.label("MDL0");
    writer.write<u32>('MDL0');                  // magic
    linker.writeReloc<u32>("MDL0", "MDL0_END"); // file size
    writer.write<u32>(11);                      // revision
    writer.write<u32>(brres_start - mdl_start);
    // linker.writeReloc<s32>("MDL0", "BRRES");    // offset to the BRRES start

    // Section offsets
    linker.writeReloc<s32>("MDL0", "RenderTree");

    linker.writeReloc<s32>("MDL0", "Bones");
    linker.writeReloc<s32>("MDL0", "Buffer_Position");
    linker.writeReloc<s32>("MDL0", "Buffer_Normal");
    linker.writeReloc<s32>("MDL0", "Buffer_Color");
    linker.writeReloc<s32>("MDL0", "Buffer_UV");

    writer.write<s32>(0); // furVec
    writer.write<s32>(0); // furPos

    linker.writeReloc<s32>("MDL0", "Materials");
    linker.writeReloc<s32>("MDL0", "Shaders");
    linker.writeReloc<s32>("MDL0", "Meshes");
    linker.writeReloc<s32>("MDL0", "TexSamplerMap");
    writer.write<s32>(0); // PaletteSamplerMap
    writer.write<s32>(0); // UserData
    writeNameForward(names, writer, mdl_start, bin.name, true);
  }
  {
    linker.label("MDL0_INFO");
    bin.info.write(writer, mdl_start);
  }

  d_cursor = writer.tell();
  u32 dicts_size = 0;
  const auto tally_dict = [&](const char* str, const auto& dict) {
    DebugReport("%s: %u entries\n", str, (unsigned)dict.size());
    if (dict.empty())
      return;
    Dictionaries.emplace(str, dicts_size + d_cursor);
    dicts_size += 24 + 16 * dict.size();
  };
  tally_dict("RenderTree", bin.bytecodes);
  tally_dict("Bones", bin.bones);
  tally_dict("Buffer_Position", bin.positions);
  tally_dict("Buffer_Normal", bin.normals);
  tally_dict("Buffer_Color", bin.colors);
  tally_dict("Buffer_UV", bin.texcoords);
  tally_dict("Materials", bin.materials);
  tally_dict("Shaders", bin.materials);
  tally_dict("Meshes", bin.meshes);

  librii::g3d::TextureSamplerMappingManager tex_sampler_mappings;

  std::set<std::string> bruh;
  for (auto& mat : bin.materials) {
    for (auto& s : mat.samplers) {
      bruh.emplace(s.texture);
    }
  }
  for (auto& tex : bruh)
    for (auto& mat : bin.materials)
      for (int s = 0; s < mat.samplers.size(); ++s)
        if (mat.samplers[s].texture == tex)
          tex_sampler_mappings.add_entry(tex, mat.name, s);
  if (tex_sampler_mappings.size()) {
    Dictionaries.emplace("TexSamplerMap", dicts_size + d_cursor);
    dicts_size += 24 + 16 * tex_sampler_mappings.size();
  }
  for (auto [key, val] : Dictionaries) {
    DebugReport("%s: %x\n", key.c_str(), (unsigned)val);
  }
  writer.skip(dicts_size);

  int sm_i = 0;
  write_dict(
      "TexSamplerMap", tex_sampler_mappings,
      [&](librii::g3d::TextureSamplerMapping& map, std::size_t start) {
        writer.write<u32>(map.entries.size());
        for (int i = 0; i < map.entries.size(); ++i) {
          tex_sampler_mappings.entries[sm_i].entries[i] = writer.tell();
          // These are placeholders
          writer.write<s32>(0, /* checkmatch */ false);
          writer.write<s32>(0, /* checkmatch */ false);
        }
        ++sm_i;
      },
      true, 4);

  write_dict(
      "RenderTree", bin.bytecodes,
      [&](const librii::g3d::ByteCodeMethod& list, std::size_t) {
        librii::g3d::ByteCodeLists::WriteStream(writer, list.commands);
      },
      true, 1);

  write_dict_mat(
      "Bones", bin.bones,
      [&](const librii::g3d::BinaryBoneData& bone, std::size_t bone_start) {
        DebugReport("Bone at %x\n", (unsigned)bone_start);
        writer.seekSet(bone_start);
        bone.write(names, writer, mdl_start);
      });

  write_dict_mat(
      "Materials", bin.materials,
      [&](const librii::g3d::BinaryMaterial& mat, std::size_t mat_start) {
        linker.label("Mat" + std::to_string(mat_start), mat_start);
        mat.writeBody(writer, mat_start, names, linker, tex_sampler_mappings);
      },
      false, 4);

  // Shaders
  {
    if (bin.tevs.size() == 0)
      return;

    u32 dict_ofs = Dictionaries.at("Shaders");
    librii::g3d::BetterDictionary _dict;
    _dict.nodes.resize(bin.materials.size());

    for (std::size_t i = 0; i < bin.tevs.size(); ++i) {
      writer.alignTo(32); //
      for (int j = 0; j < bin.materials.size(); ++j) {
        if (bin.materials[j].tevId == i) {
          _dict.nodes[j].stream_pos = writer.tell();
          _dict.nodes[j].name = bin.materials[j].name;
        }
      }
      const auto backpatch = writePlaceholder(writer); // size
      writer.write<s32>(mdl_start - backpatch);        // mdl0 offset
      WriteShader(linker, writer, bin.tevs[i], backpatch, i);
      writeOffsetBackpatch(writer, backpatch, backpatch);
    }
    {
      oishii::Jump<oishii::Whence::Set, oishii::Writer> d(writer, dict_ofs);
      linker.label("Shaders");
      WriteDictionary(_dict, writer, names);
    }
  }
  write_dict(
      "Meshes", bin.meshes,
      [&](const librii::g3d::PolygonData& mesh, std::size_t mesh_start) {
        auto err = WriteMesh(writer, mesh, bin, mesh_start, names);
        if (!err.empty()) {
          fprintf(stderr, "Error: %s\n", err.c_str());
          abort();
        }
      },
      false, 32);

  write_dict(
      "Buffer_Position", bin.positions,
      [&](const librii::g3d::PositionBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_Normal", bin.normals,
      [&](const librii::g3d::NormalBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_Color", bin.colors,
      [&](const librii::g3d::ColorBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_UV", bin.texcoords,
      [&](const librii::g3d::TextureCoordinateBuffer& buf,
          std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);

  linker.writeChildren();
  linker.label("MDL0_END");
  linker.resolve();
}

} // namespace

void BinaryModel::write(oishii::Writer& writer, NameTable& names,
                        std::size_t brres_start) {
  writeModel(*this, writer, names, brres_start);
}

} // namespace librii::g3d
