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

#include <librii/g3d/io/WiiTrig.hpp>
#include <librii/math/mtx.hpp>
#include <librii/math/srt3.hpp>

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
    rsl::SafeReader& reader, auto&& vcc, auto&& vbt) {
  // librii::gx::VertexComponentCount::Color
  using VCC = std::remove_cvref_t<decltype(vcc)>;
  // librii::gx::VertexBufferType::Color
  using VBT = std::remove_cvref_t<decltype(vbt)>;
  const auto start = reader.tell();
  out.mEntries.clear();

  TRY(reader.U32()); // size
  TRY(reader.U32()); // mdl0 offset
  const auto startOfs = TRY(reader.S32());
  out.mName = TRY(reader.StringOfs(start));
  out.mId = TRY(reader.U32());
  out.mQuantize.mComp =
      librii::gx::VertexComponentCount(static_cast<VCC>(TRY(reader.U32())));
  out.mQuantize.mType =
      librii::gx::VertexBufferType(static_cast<VBT>(TRY(reader.U32())));
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
  TRY(readDict(
      secOfs.ofsBuffers.position, [&](const librii::g3d::BetterNode& dnode) {
        return readGenericBuffer(positions.emplace_back(), reader,
                                 librii::gx::VertexComponentCount::Position{},
                                 librii::gx::VertexBufferType::Generic{});
      }));
  TRY(readDict(
      secOfs.ofsBuffers.normal, [&](const librii::g3d::BetterNode& dnode) {
        return readGenericBuffer(normals.emplace_back(), reader,
                                 librii::gx::VertexComponentCount::Normal{},
                                 librii::gx::VertexBufferType::Generic{});
      }));
  TRY(readDict(
      secOfs.ofsBuffers.color, [&](const librii::g3d::BetterNode& dnode) {
        return readGenericBuffer(colors.emplace_back(), reader,
                                 librii::gx::VertexComponentCount::Color{},
                                 librii::gx::VertexBufferType::Color{});
      }));
  TRY(readDict(secOfs.ofsBuffers.uv, [&](const librii::g3d::BetterNode& dnode) {
    return readGenericBuffer(
        texcoords.emplace_back(), reader,
        librii::gx::VertexComponentCount::TextureCoordinate{},
        librii::gx::VertexBufferType::Generic{});
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
    oishii::Writer& writer, u32 header_start, NameTable& names, auto&& vc,
    auto&& vt) {
  const auto backpatch_array_ofs = writePlaceholder(writer);
  writeNameForward(names, writer, header_start, buf.mName);
  writer.write<u32>(buf.mId);
  writer.write<u32>(static_cast<u32>(vc));
  writer.write<u32>(static_cast<u32>(vt));
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
        writeGenericBuffer(buf, writer, buf_start, names,
                           buf.mQuantize.mComp.position,
                           buf.mQuantize.mType.generic);
      },
      false, 32);
  write_dict(
      "Buffer_Normal", bin.normals,
      [&](const librii::g3d::NormalBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names,
                           buf.mQuantize.mComp.normal,
                           buf.mQuantize.mType.generic);
      },
      false, 32);
  write_dict(
      "Buffer_Color", bin.colors,
      [&](const librii::g3d::ColorBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names,
                           buf.mQuantize.mComp.color,
                           buf.mQuantize.mType.color);
      },
      false, 32);
  write_dict(
      "Buffer_UV", bin.texcoords,
      [&](const librii::g3d::TextureCoordinateBuffer& buf,
          std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names,
                           buf.mQuantize.mComp.texcoord,
                           buf.mQuantize.mType.generic);
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

//
//
// Intermediate model
//
//

#pragma region Bones

u32 computeFlag(const librii::g3d::BoneData& data,
                std::span<const librii::g3d::BoneData> all,
                librii::g3d::ScalingRule scalingRule) {
  u32 flag = 0;
  const std::array<float, 3> scale{data.mScaling.x, data.mScaling.y,
                                   data.mScaling.z};
  if (rsl::RangeIsHomogenous(scale)) {
    flag |= 0x10;
    if (data.mScaling == glm::vec3{1.0f, 1.0f, 1.0f})
      flag |= 8;
  }
  if (data.mRotation == glm::vec3{0.0f, 0.0f, 0.0f})
    flag |= 4;
  if (data.mTranslation == glm::vec3{0.0f, 0.0f, 0.0f})
    flag |= 2;
  if ((flag & (2 | 4 | 8)) == (2 | 4 | 8))
    flag |= 1;
  bool has_ssc_below = false;
  {
    std::vector<s32> stack;
    for (auto c : data.mChildren)
      stack.push_back(c);
    while (!stack.empty()) {
      auto& elem = all[stack.back()];
      stack.resize(stack.size() - 1);
      if (elem.ssc) {
        has_ssc_below = true;
      }
      // It seems it's not actually a recursive flag?

      // for (auto c : elem.mChildren)
      //   stack.push_back(c);
    }
    if (has_ssc_below) {
      flag |= 0x40;
    }
  }
  if (data.ssc)
    flag |= 0x20;
  if (scalingRule == librii::g3d::ScalingRule::XSI)
    flag |= 0x80;
  if (data.visible)
    flag |= 0x100;
  // TODO: Check children?

  if (data.displayMatrix)
    flag |= 0x200;
  // TODO: 0x400 check parents
  return flag;
}
// Call this last
void setFromFlag(librii::g3d::BoneData& data, u32 flag) {
  // TODO: Validate items
  data.ssc = (flag & 0x20) != 0;

  // Instead refer to scalingRule == SOFTIMAGE
  // data.classicScale = (flag & 0x80) == 0;

  data.visible = (flag & 0x100) != 0;
  // Just trust it at this point
  data.displayMatrix = (flag & 0x200) != 0;
}

librii::math::SRT3 getSrt(const librii::g3d::BinaryBoneData& bone) {
  return {
      .scale = bone.scale,
      .rotation = bone.rotate,
      .translation = bone.translate,
  };
}
librii::math::SRT3 getSrt(const librii::g3d::BoneData& bone) {
  return {
      .scale = bone.mScaling,
      .rotation = bone.mRotation,
      .translation = bone.mTranslation,
  };
}

s32 parentOf(const librii::g3d::BinaryBoneData& bone) { return bone.parent_id; }
s32 parentOf(const librii::g3d::BoneData& bone) { return bone.mParent; }

s32 ssc(const librii::g3d::BinaryBoneData& bone) { return bone.flag & 0x20; }
s32 ssc(const librii::g3d::BoneData& bone) { return bone.ssc; }

using WiiFloat = f64;

// COLUMN-MAJOR IMPLEMENTATION
void CalcEnvelopeContribution(glm::mat4& thisMatrix, glm::vec3& thisScale,
                              const auto& node, const glm::mat4& parentMtx,
                              const glm::vec3& parentScale,
                              librii::g3d::ScalingRule scalingRule) {
  auto srt = getSrt(node);
  if (ssc(node)) {
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = (WiiFloat)srt.translation.x * (WiiFloat)parentScale.x;
    thisMatrix[3][1] = (WiiFloat)srt.translation.y * (WiiFloat)parentScale.y;
    thisMatrix[3][2] = (WiiFloat)srt.translation.z * (WiiFloat)parentScale.z;
    // PSMTXConcat(parentMtx, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = parentMtx * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(parentMtx, thisMatrix);
    thisScale.x = srt.scale.x;
    thisScale.y = srt.scale.y;
    thisScale.z = srt.scale.z;
  } else if (scalingRule ==
             librii::g3d::ScalingRule::XSI) { // CLASSIC_SCALE_OFF
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = (WiiFloat)srt.translation.x * (WiiFloat)parentScale.x;
    thisMatrix[3][1] = (WiiFloat)srt.translation.y * (WiiFloat)parentScale.y;
    thisMatrix[3][2] = (WiiFloat)srt.translation.z * (WiiFloat)parentScale.z;
    // PSMTXConcat(parentMtx, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = parentMtx * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(parentMtx, thisMatrix);
    thisScale.x = (WiiFloat)srt.scale.x * (WiiFloat)parentScale.x;
    thisScale.y = (WiiFloat)srt.scale.y * (WiiFloat)parentScale.y;
    thisScale.z = (WiiFloat)srt.scale.z * (WiiFloat)parentScale.z;
  } else {
    glm::mat4x3 scratch;
    // Mtx_scale(/*out*/ &scratch, parentMtx, parentScale);
    librii::g3d::Mtx_scale(/*out*/ scratch, parentMtx, parentScale);
    // scratch = glm::scale(parentMtx, parentScale);
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = srt.translation.x;
    thisMatrix[3][1] = srt.translation.y;
    thisMatrix[3][2] = srt.translation.z;
    // PSMTXConcat(scratch, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = scratch * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(scratch, thisMatrix);
    thisScale.x = srt.scale.x;
    thisScale.y = srt.scale.y;
    thisScale.z = srt.scale.z;
  }
}

// SLOW IMPLEMENTATION
inline glm::mat4 calcSrtMtx(const auto& bone, auto&& bones,
                            librii::g3d::ScalingRule scalingRule) {
  std::vector<s32> path;
  auto* it = &bone;
  while (true) {
    s32 parentIndex = parentOf(*it);
    if (parentIndex < 0 || parentIndex >= std::ranges::size(bones)) {
      break;
    }
    path.push_back(parentIndex);
    it = &bones[parentIndex];
  }

  glm::mat4 mat(1.0f);
  glm::vec3 scl(1.0f, 1.0f, 1.0f);
  for (int i = 0; i < path.size(); ++i) {
    auto index = path[path.size() - 1 - i];
    auto& thisBone = bones[index];

    glm::mat4 thisMatrix(1.0f);
    glm::vec3 thisScale(1.0f, 1.0f, 1.0f);
    CalcEnvelopeContribution(/*out*/ thisMatrix, /*out*/ thisScale, thisBone,
                             mat, scl, scalingRule);
    mat = thisMatrix;
    scl = thisScale;
  }
  glm::mat4 thisMatrix(1.0f);
  glm::vec3 thisScale(1.0f, 1.0f, 1.0f);
  {
    CalcEnvelopeContribution(/*out*/ thisMatrix, /*out*/ thisScale, bone, mat,
                             scl, scalingRule);
  }
  // return glm::scale(thisMatrix, thisScale);
  glm::mat4x3 tmp;
  librii::g3d::Mtx_scale(tmp, thisMatrix, thisScale);
  return tmp;
}

librii::g3d::BoneData fromBinaryBone(const librii::g3d::BinaryBoneData& bin,
                                     auto&& bones, kpi::IOContext& ctx_,
                                     librii::g3d::ScalingRule scalingRule) {
  auto ctx = ctx_.sublet("Bone " + bin.name);
  librii::g3d::BoneData bone;
  bone.mName = bin.name;
  printf("%s\n", bone.mName.c_str());
  // TODO: Verify matrixId
  bone.billboardType = bin.billboardType;
  // TODO: refId
  bone.mScaling = bin.scale;
  bone.mRotation = bin.rotate;
  bone.mTranslation = bin.translate;

  bone.mVolume = bin.aabb;

  bone.mParent = bin.parent_id;
  // Skip sibling and child links -- we recompute it all

  auto modelMtx = calcSrtMtx(bin, bones, scalingRule);
  auto modelMtx34 = glm::mat4x3(modelMtx);

  auto invModelMtx =
      librii::g3d::MTXInverse(modelMtx).value_or(glm::mat4(0.0f));
  auto invModelMtx34 = glm::mat4x3(invModelMtx);

  // auto test = jensen_shannon_divergence(bin.modelMtx, bin.inverseModelMtx);
  // auto test2 = jensen_shannon_divergence(bin.modelMtx, glm::mat4(1.0f));
  auto divergeM =
      librii::math::jensen_shannon_divergence(bin.modelMtx, modelMtx34);
  auto divergeI = librii::math::jensen_shannon_divergence(bin.inverseModelMtx,
                                                          invModelMtx34);
  // assert(bin.modelMtx == modelMtx34);
  // assert(bin.inverseModelMtx == invModelMtx34);
  ctx.require(bin.modelMtx == modelMtx34,
              "Incorrect envelope matrix (Jensen-Shannon divergence: {})",
              divergeM);
  ctx.require(bin.inverseModelMtx == invModelMtx34,
              "Incorrect inverse model matrix (Jensen-Shannon divergence: {})",
              divergeI);
  bool classic = !static_cast<bool>(bin.flag & 0x80);
  if (classic != (scalingRule != librii::g3d::ScalingRule::XSI)) {
    ctx.error(std::format("Bone is configured as {} but model is {}",
                          !classic ? "XSI" : "non-XSI",
                          magic_enum::enum_name(scalingRule)));
  }
  setFromFlag(bone, bin.flag);
  return bone;
}

librii::g3d::BinaryBoneData
toBinaryBone(const librii::g3d::BoneData& bone,
             std::span<const librii::g3d::BoneData> bones, u32 bone_id,
             librii::g3d::ScalingRule scalingRule, s32 matrixId) {
  librii::g3d::BinaryBoneData bin;
  bin.name = bone.mName;
  bin.matrixId = matrixId;

  // TODO: Fix
  bin.flag = computeFlag(bone, bones, scalingRule);
  bin.id = bone_id;

  bin.billboardType = bone.billboardType;
  bin.ancestorBillboardBone = 0; // TODO: ref
  bin.scale = bone.mScaling;
  bin.rotate = bone.mRotation;
  bin.translate = bone.mTranslation;
  bin.aabb = bone.mVolume;

  // Parent, Child, Left, Right
  bin.parent_id = bone.mParent;
  bin.child_first_id = -1;
  bin.sibling_left_id = -1;
  bin.sibling_right_id = -1;

  if (bone.mChildren.size()) {
    bin.child_first_id = bone.mChildren[0];
  }
  if (bone.mParent != -1) {
    auto& siblings = bones[bone.mParent].mChildren;
    auto it = std::find(siblings.begin(), siblings.end(), bone_id);
    assert(it != siblings.end());
    // Not a cyclic linked list
    bin.sibling_left_id = it == siblings.begin() ? -1 : *(it - 1);
    bin.sibling_right_id = it == siblings.end() - 1 ? -1 : *(it + 1);
  }

  auto modelMtx = calcSrtMtx(bone, bones, scalingRule);
  auto modelMtx34 = glm::mat4x3(modelMtx);

  bin.modelMtx = modelMtx34;
  bin.inverseModelMtx =
      librii::g3d::MTXInverse(modelMtx34).value_or(glm::mat4x3(0.0f));
  return bin;
}

#pragma endregion

#pragma region librii->Editor

// Handles all incoming bytecode on a method and applies it to the model.
//
// clang-format off
//
// - DrawOpa(material, bone, mesh) -> assert(material.xlu) && bone.addDrawCall(material, mesh)
// - DrawXlu(material, bone, mesh) -> assert(!material.xlu) && bone.addDrawCall(material, mesh)
// - EvpMtx(mtxId, boneId) -> model.insertDrawMatrix(mtxId, { .bone = boneId, .weight = 100% })
// - NodeMix(mtxId, [(mtxId, ratio)]) -> model.insertDrawMatrix(mtxId, ... { .bone = LUT[mtxId], .ratio = ratio })
//
// clang-format on
class ByteCodeHelper {
  using B = librii::g3d::ByteCodeLists;

public:
  ByteCodeHelper(const ByteCodeMethod& method_, Model& mdl_,
                 const BinaryModel& bmdl_, kpi::IOContext& ctx_)
      : method(method_), mdl(mdl_), binary_mdl(bmdl_), ctx(ctx_) {}

  void onDraw(const B::Draw& draw) {
    BoneData::DisplayCommand disp{
        .mMaterial = static_cast<u32>(draw.matId),
        .mPoly = static_cast<u32>(draw.polyId),
        .mPrio = draw.prio,
    };
    auto boneIdx = draw.boneId;
    if (boneIdx > mdl.bones.size()) {
      ctx.error("Invalid bone index in render command");
      boneIdx = 0;
      ctx.transaction.state = kpi::TransactionState::Failure;
    }

    if (disp.mMaterial > mdl.meshes.size()) {
      ctx.error("Invalid material index in render command");
      disp.mMaterial = 0;
      ctx.transaction.state = kpi::TransactionState::Failure;
    }

    if (disp.mPoly > mdl.meshes.size()) {
      ctx.error("Invalid mesh index in render command");
      disp.mPoly = 0;
      ctx.transaction.state = kpi::TransactionState::Failure;
    }

    mdl.bones[boneIdx].mDisplayCommands.push_back(disp);

    // While with this setup, materials could be XLU and OPA, in
    // practice, they're not.
    //
    // Warn the user if a material is flagged as OPA/XLU but doesn't exist
    // in the corresponding draw list.
    auto& mat = mdl.materials[disp.mMaterial];
    auto& poly = mdl.meshes[disp.mPoly];
    {
      const bool xlu_mat = mat.flag & 0x80000000;
      if ((method.name == "DrawOpa" && xlu_mat) ||
          (method.name == "DrawXlu" && !xlu_mat)) {
        kpi::IOContext mc = ctx.sublet("materials").sublet(mat.name);
        mc.request(
            false,
            "Material {} (#{}) is rendered in the {} pass (with mesh {} #{}), "
            "but is marked as {}",
            mat.name, disp.mMaterial, xlu_mat ? "Opaque" : "Translucent",
            disp.mPoly, poly.mName, !xlu_mat ? "Opaque" : "Translucent");
      }
    }
    // And ultimately reset the flag to its proper value.
    mat.xlu = method.name == "DrawXlu";
  }

  void onNodeDesc(const B::NodeDescendence& desc) {
    auto& bin = binary_mdl.bones[desc.boneId];
    auto& bone = mdl.bones[desc.boneId];
    const auto matrixId = bin.matrixId;

    auto parent_id =
        binary_mdl.info.mtxToBoneLUT.mtxIdToBoneId[desc.parentMtxId];
    if (bone.mParent != -1 && parent_id >= 0) {
      bone.mParent = parent_id;
    }

    if (matrixId >= mdl.matrices.size()) {
      mdl.matrices.resize(matrixId + 1);
    }
    mdl.matrices[matrixId].mWeights.emplace_back(desc.boneId, 1.0f);
  }

  // Either-or: A matrix is either single-bound (EVP) or multi-influence
  // (NODEMIX)
  void onEvpMtx(const B::EnvelopeMatrix& evp) {
    auto& drw = insertMatrix(evp.mtxId);
    drw.mWeights = {{evp.nodeId, 1.0f}};
  }

  Result<void> onNodeMix(const B::NodeMix& mix) {
    auto& drw = insertMatrix(mix.mtxId);
    auto range = mix.blendMatrices |
                 std::views::transform([&](const B::NodeMix::BlendMtx& blend)
                                           -> Result<DrawMatrix::MatrixWeight> {
                   int boneIndex =
                       binary_mdl.info.mtxToBoneLUT.mtxIdToBoneId[blend.mtxId];
                   EXPECT(boneIndex != -1);
                   return DrawMatrix::MatrixWeight{static_cast<u32>(boneIndex),
                                                   blend.ratio};
                 });
    for (auto&& w : range) {
      drw.mWeights.push_back(TRY(w));
    }
    return {};
  }

private:
  DrawMatrix& insertMatrix(std::size_t index) {
    if (mdl.matrices.size() <= index) {
      mdl.matrices.resize(index + 1);
    }
    return mdl.matrices[index];
  }
  const ByteCodeMethod& method;
  Model& mdl;
  const BinaryModel& binary_mdl;
  kpi::IOContext& ctx;
};

Result<void> processModel(const BinaryModel& binary_model,
                          kpi::LightIOTransaction& transaction,
                          std::string_view transaction_path, Model& mdl) {
  using namespace librii::g3d;
  if (transaction.state == kpi::TransactionState::Failure) {
    return {};
  }
  kpi::IOContext ctx(std::string(transaction_path) + "//MDL0 " +
                         binary_model.name,
                     transaction);

  mdl.name = binary_model.name;

  u32 numDisplayMatrices = 1;
  // Assign info
  {
    const auto& info = binary_model.info;
    mdl.info.scalingRule = info.scalingRule;
    mdl.info.texMtxMode = info.texMtxMode;
    // Validate numVerts, numTris
    {
      auto [computedNumVerts, computedNumTris] =
          librii::gx::computeVertTriCounts(binary_model.meshes);
      ctx.request(
          computedNumVerts == info.numVerts,
          "Model header specifies {} vertices, but the file only has {}.",
          info.numVerts, computedNumVerts);
      ctx.request(
          computedNumTris == info.numTris,
          "Model header specifies {} triangles, but the file only has {}.",
          info.numTris, computedNumTris);
    }
    mdl.info.sourceLocation = info.sourceLocation;
    {
      auto displayMatrices = librii::gx::computeDisplayMatricesSubset(
          binary_model.meshes, binary_model.bones,
          [](auto& x, int i) { return x.matrixId; });
      ctx.request(info.numViewMtx == displayMatrices.size(),
                  "Model header specifies {} display matrices, but the mesh "
                  "data only references {} display matrices.",
                  info.numViewMtx, displayMatrices.size());
      auto ref = librii::gx::computeShapeMtxRef(binary_model.meshes);
      numDisplayMatrices = ref.size();
    }
    {
      auto needsNormalMtx =
          std::any_of(binary_model.meshes.begin(), binary_model.meshes.end(),
                      [](auto& m) { return m.needsNormalMtx(); });
      ctx.request(
          info.normalMtxArray == needsNormalMtx,
          needsNormalMtx
              ? "Model header unecessarily burdens the runtime library "
                "with bone-normal-matrix computation"
              : "Model header does not tell the runtime library to maintain "
                "bone normal matrix arrays, although some meshes need it");
    }
    {
      auto needsTexMtx =
          std::any_of(binary_model.meshes.begin(), binary_model.meshes.end(),
                      [](auto& m) { return m.needsTextureMtx(); });
      ctx.request(
          info.texMtxArray == needsTexMtx,
          needsTexMtx
              ? "Model header unecessarily burdens the runtime library "
                "with bone-texture-matrix computation"
              : "Model header does not tell the runtime library to maintain "
                "bone texture matrix arrays, although some meshes need it");
    }
    ctx.request(!info.boundVolume,
                "Model specifies bounding data should be used");
    mdl.info.evpMtxMode = info.evpMtxMode;
    mdl.info.min = info.min;
    mdl.info.max = info.max;
    // Validate mtxToBoneLUT
    {
      const auto& lut = info.mtxToBoneLUT.mtxIdToBoneId;
      for (size_t i = 0; i < binary_model.bones.size(); ++i) {
        const auto& bone = binary_model.bones[i];
        if (bone.matrixId > lut.size()) {
          ctx.error(
              std::format("Bone {} specifies a matrix ID of {}, but the matrix "
                          "LUT only specifies {} matrices total.",
                          bone.name, bone.matrixId, lut.size()));
          continue;
        }
        ctx.request(lut[bone.matrixId] == i,
                    "Bone {} (#{}) declares ownership of Matrix{}. However, "
                    "Matrix{} does not register this bone as its owner. "
                    "Rather, it specifies an owner ID of {}.",
                    bone.name, i, bone.matrixId, bone.matrixId,
                    lut[bone.matrixId]);
      }
    }
  }

  mdl.bones.resize(0);
  for (size_t i = 0; i < binary_model.bones.size(); ++i) {
    EXPECT(binary_model.bones[i].id == i);
    mdl.bones.emplace_back() =
        fromBinaryBone(binary_model.bones[i], binary_model.bones, ctx,
                       binary_model.info.scalingRule);
  }

  mdl.positions = binary_model.positions;
  mdl.normals = binary_model.normals;
  mdl.colors = binary_model.colors;
  mdl.texcoords = binary_model.texcoords;
  // TODO: Fur
  for (auto& mat : binary_model.materials) {
    auto ok = TRY(librii::g3d::fromBinMat(mat, &mat.tev));
    mdl.materials.emplace_back() = ok;
  }
  mdl.meshes = binary_model.meshes;
  // Process bytecode: Apply to materials/bones/draw matrices
  for (auto& method : binary_model.bytecodes) {
    ByteCodeHelper helper(method, mdl, binary_model, ctx);
    for (auto& command : method.commands) {
      if (auto* draw = std::get_if<ByteCodeLists::Draw>(&command)) {
        helper.onDraw(*draw);
      } else if (auto* desc =
                     std::get_if<ByteCodeLists::NodeDescendence>(&command)) {
        helper.onNodeDesc(*desc);
      } else if (auto* evp =
                     std::get_if<ByteCodeLists::EnvelopeMatrix>(&command)) {
        helper.onEvpMtx(*evp);
      } else if (auto* mix = std::get_if<ByteCodeLists::NodeMix>(&command)) {
        TRY(helper.onNodeMix(*mix));
      } else {
        // TODO: Other bytecodes
        EXPECT(false, "Unexpected bytecode");
      }
    }
  }

  // Assumes all display matrices contiguously precede non-display matrices.
  // Trim non-display matrices to reach numViewMtx.
  // Can't trust the header -- BrawlBox doesn't properly set it.
  EXPECT(numDisplayMatrices <= mdl.matrices.size());
  mdl.matrices.resize(numDisplayMatrices);

  // Recompute parent-child relationships
  for (size_t i = 0; i < mdl.bones.size(); ++i) {
    auto& bone = mdl.bones[i];
    if (bone.mParent == -1) {
      continue;
    }
    auto& parent = mdl.bones[bone.mParent];
    parent.mChildren.push_back(i);
  }
  return {};
}

#pragma endregion

#pragma region Editor->librii

void BuildRenderLists(const Model& mdl,
                      std::vector<librii::g3d::ByteCodeMethod>& renderLists,
                      // Expanded to fit non-draw bone matrices
                      std::span<const DrawMatrix> drawMatrices,
                      // Maps bones to above drawMatrix span
                      std::span<const u32> boneToMatrix) {
  librii::g3d::ByteCodeMethod nodeTree{.name = "NodeTree"};
  librii::g3d::ByteCodeMethod nodeMix{.name = "NodeMix"};
  librii::g3d::ByteCodeMethod drawOpa{.name = "DrawOpa"};
  librii::g3d::ByteCodeMethod drawXlu{.name = "DrawXlu"};

  for (size_t i = 0; i < mdl.bones.size(); ++i) {
    for (const auto& draw : mdl.bones[i].mDisplayCommands) {
      librii::g3d::ByteCodeLists::Draw cmd{
          .matId = static_cast<u16>(draw.mMaterial),
          .polyId = static_cast<u16>(draw.mPoly),
          .boneId = static_cast<u16>(i),
          .prio = draw.mPrio,
      };
      bool xlu = draw.mMaterial < std::size(mdl.materials) &&
                 mdl.materials[draw.mMaterial].xlu;
      (xlu ? &drawXlu : &drawOpa)->commands.push_back(cmd);
    }
    auto parent = mdl.bones[i].mParent;
    assert(parent < std::ssize(mdl.bones));
    librii::g3d::ByteCodeLists::NodeDescendence desc{
        .boneId = static_cast<u16>(i),
        .parentMtxId = static_cast<u16>(parent >= 0 ? boneToMatrix[parent] : 0),
    };
    nodeTree.commands.push_back(desc);
  }

  auto write_drw = [&](const DrawMatrix& drw, size_t i) {
    if (drw.mWeights.size() > 1) {
      librii::g3d::ByteCodeLists::NodeMix mix{
          .mtxId = static_cast<u16>(i),
      };
      for (auto& weight : drw.mWeights) {
        assert(weight.boneId < mdl.bones.size());
        mix.blendMatrices.push_back(
            librii::g3d::ByteCodeLists::NodeMix::BlendMtx{
                .mtxId = static_cast<u16>(boneToMatrix[weight.boneId]),
                .ratio = weight.weight,
            });
      }
      nodeMix.commands.push_back(mix);
    } else {
      assert(drw.mWeights[0].boneId < mdl.bones.size());
      librii::g3d::ByteCodeLists::EnvelopeMatrix evp{
          .mtxId = static_cast<u16>(i),
          .nodeId = static_cast<u16>(drw.mWeights[0].boneId),
      };
      nodeMix.commands.push_back(evp);
    }
  };

  // TODO: Better heuristic. When do we *need* NodeMix? Presumably when at least
  // one bone is weighted to a matrix that is not a bone directly? Or when that
  // bone is influenced by another bone?
  bool needs_nodemix = false;
  for (auto& mtx : drawMatrices) {
    if (mtx.mWeights.size() > 1) {
      needs_nodemix = true;
      break;
    }
  }

  if (needs_nodemix) {
    // Bones come first
    for (size_t i = 0; i < mdl.bones.size(); ++i) {
      auto& drw = drawMatrices[boneToMatrix[i]];
      write_drw(drw, boneToMatrix[i]);
    }
    for (size_t i = 0; i < mdl.matrices.size(); ++i) {
      auto& drw = drawMatrices[i];
      if (drw.mWeights.size() == 1) {
        // Written in pre-pass
        continue;
      }
      write_drw(drw, i);
    }
  }

  renderLists.push_back(nodeTree);
  if (!nodeMix.commands.empty()) {
    renderLists.push_back(nodeMix);
  }
  if (!drawOpa.commands.empty()) {
    renderLists.push_back(drawOpa);
  }
  if (!drawXlu.commands.empty()) {
    renderLists.push_back(drawXlu);
  }
}

struct ShaderAllocator {
  void alloc(const librii::g3d::G3dShader& shader) {
    auto found = std::find(shaders.begin(), shaders.end(), shader);
    if (found == shaders.end()) {
      matToShaderMap.emplace_back(shaders.size());
      shaders.emplace_back(shader);
    } else {
      matToShaderMap.emplace_back(found - shaders.begin());
    }
  }

  int find(const librii::g3d::G3dShader& shader) const {
    if (const auto found = std::find(shaders.begin(), shaders.end(), shader);
        found != shaders.end()) {
      return std::distance(shaders.begin(), found);
    }

    return -1;
  }

  auto size() const { return shaders.size(); }

  std::string getShaderIDName(const librii::g3d::G3dShader& shader) const {
    const int found = find(shader);
    assert(found >= 0);
    return "Shader" + std::to_string(found);
  }

  std::vector<librii::g3d::G3dShader> shaders;
  std::vector<u32> matToShaderMap;
};

librii::g3d::BinaryModel toBinaryModel(const Model& mdl) {
  std::set<s16> shapeRefMtx = computeShapeMtxRef(mdl.meshes);
  DebugPrint("shapeRefMtx: {}, Before: {}", shapeRefMtx.size(),
             mdl.matrices.size());
  assert(shapeRefMtx.size() == mdl.matrices.size());
  std::vector<DrawMatrix> drawMatrices = mdl.matrices;
  std::vector<u32> boneToMatrix;
  DebugPrint("# Bones = {}", mdl.bones.size());
  for (size_t i = 0; i < mdl.bones.size(); ++i) {
    int it = -1;
    for (size_t j = 0; j < mdl.matrices.size(); ++j) {
      auto& mtx = mdl.matrices[j];
      if (mtx.mWeights.size() == 1 && mtx.mWeights[0].boneId == i) {
        it = j;
        break;
      }
    }
    if (it == -1) {
      boneToMatrix.push_back(drawMatrices.size());
      DrawMatrix mtx{.mWeights = {{static_cast<u32>(i), 1.0f}}};
      DebugPrint("Adding drawmtx {} for bone {} since no singlebound found",
                 drawMatrices.size(), i);
      drawMatrices.push_back(mtx);
    } else {
      boneToMatrix.push_back(it);
    }
  }
  auto getMatrixId = [&](auto& x, int i) { return boneToMatrix[i]; };
  std::set<s16> displayMatrices =
      gx::computeDisplayMatricesSubset(mdl.meshes, mdl.bones, getMatrixId);
  DebugPrint("boneToMatrix: {}, displayMatrices: {}, drawMatrices: {}",
             boneToMatrix.size(), displayMatrices.size(), drawMatrices.size());

  ShaderAllocator shader_allocator;
  for (auto& mat : mdl.materials) {
    librii::g3d::G3dShader shader(mat);
    shader_allocator.alloc(shader);
  }
  std::vector<librii::g3d::BinaryTev> tevs;
  for (size_t i = 0; i < shader_allocator.shaders.size(); ++i) {
    tevs.push_back(toBinaryTev(shader_allocator.shaders[i], i));
  }

  auto bones = mdl.bones | rsl::ToList<librii::g3d::BoneData>();
  auto to_binary_bone = [&](auto tuple) -> BinaryBoneData {
    auto [index, value] = tuple;
    return toBinaryBone(value, bones, index, mdl.info.scalingRule,
                        boneToMatrix[index]);
  };
  auto to_binary_mat = [&](auto tuple) {
    auto [index, value] = tuple;
    return librii::g3d::toBinMat(value, index);
  };
  librii::g3d::BinaryModel bin{
      .name = mdl.name,
      .bones = mdl.bones          // Start with the bones
               | rsl::enumerate() // Convert to [i, bone] tuples

               // Needed for some reason
               | rsl::ToList()
               //

               | std::views::transform(to_binary_bone) // Convert to BinaryBone
               | rsl::ToList<BinaryBoneData>(),        // And back to vector
      .positions = mdl.positions,
      .normals = mdl.normals,
      .colors = mdl.colors,
      .texcoords = mdl.texcoords,
      .materials = mdl.materials      //
                   | rsl::enumerate() //

                   // Needed for some reason
                   | rsl::ToList()
                   //

                   | std::views::transform(to_binary_mat) //
                   | rsl::ToList(),
      .tevs = tevs,
      .meshes = mdl.meshes,
  };

  for (size_t i = 0; i < shader_allocator.matToShaderMap.size(); ++i) {
    bin.materials[i].tevId = shader_allocator.matToShaderMap[i];
  }

  // Compute ModelInfo
  {
    const auto [nVert, nTri] = librii::gx::computeVertTriCounts(mdl.meshes);
    bool nrmMtx = std::ranges::any_of(
        mdl.meshes, [](auto& m) { return m.needsNormalMtx(); });
    bool texMtx = std::ranges::any_of(
        mdl.meshes, [](auto& m) { return m.needsTextureMtx(); });
    librii::g3d::BinaryModelInfo info{
        .scalingRule = mdl.info.scalingRule,
        .texMtxMode = mdl.info.texMtxMode,
        .numVerts = nVert,
        .numTris = nTri,
        .sourceLocation = mdl.info.sourceLocation,
        .numViewMtx = static_cast<u32>(displayMatrices.size()),
        .normalMtxArray = nrmMtx,
        .texMtxArray = texMtx,
        .boundVolume = false,
        .evpMtxMode = mdl.info.evpMtxMode,
        .min = mdl.info.min,
        .max = mdl.info.max,
    };
    {
      // Matrix -> Bone LUT
      auto& lut = info.mtxToBoneLUT.mtxIdToBoneId;
      lut.resize(drawMatrices.size());
      std::ranges::fill(lut, -1);
      for (const auto& [i, bone] : rsl::enumerate(bin.bones)) {
        assert(bone.matrixId < lut.size());
        lut[bone.matrixId] = i;
      }
    }
    bin.info = info;
  }
  BuildRenderLists(mdl, bin.bytecodes, drawMatrices, boneToMatrix);

  return bin;
}

#pragma endregion

Result<Model> Model::from(const BinaryModel& model,
                          kpi::LightIOTransaction& transaction,
                          std::string_view transaction_path) {
  Model tmp{};
  TRY(processModel(model, transaction, transaction_path, tmp));
  return tmp;
}
Result<BinaryModel> Model::binary() const { return toBinaryModel(*this); }

} // namespace librii::g3d
