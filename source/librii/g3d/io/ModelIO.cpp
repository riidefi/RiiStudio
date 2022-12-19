#include "ModelIO.hpp"

#include "CommonIO.hpp"
#include <librii/g3d/io/BoneIO.hpp>
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

// Enable DictWriteIO (which is half broken: it works for reading, but not
// writing)
namespace librii::g3d {
using namespace bad;
}

///// Headers of glm_io.hpp
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vendor/glm/vec2.hpp>
#include <vendor/glm/vec3.hpp>

namespace librii::g3d {

///// Bring body of glm_io.hpp into namespace (without headers)
#include <core/util/glm_io.hpp>
inline void operator<<(librii::gx::Color& out, oishii::BinaryReader& reader) {
  out = librii::gx::readColorComponents(
      reader, librii::gx::VertexBufferType::Color::rgba8);
}

inline void operator>>(const librii::gx::Color& out, oishii::Writer& writer) {
  librii::gx::writeColorComponents(writer, out,
                                   librii::gx::VertexBufferType::Color::rgba8);
}

template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
std::string readGenericBuffer(
    librii::g3d::GenericBuffer<T, HasMinimum, HasDivisor, kind>& out,
    oishii::BinaryReader& reader) {
  const auto start = reader.tell();
  out.mEntries.clear();

  reader.read<u32>(); // size
  reader.read<u32>(); // mdl0 offset
  const auto startOfs = reader.read<s32>();
  out.mName = readName(reader, start);
  out.mId = reader.read<u32>();
  out.mQuantize.mComp = librii::gx::VertexComponentCount(
      static_cast<librii::gx::VertexComponentCount::Normal>(
          reader.read<u32>()));
  out.mQuantize.mType = librii::gx::VertexBufferType(
      static_cast<librii::gx::VertexBufferType::Color>(reader.read<u32>()));
  if (HasDivisor) {
    out.mQuantize.divisor = reader.read<u8>();
    out.mQuantize.stride = reader.read<u8>();
  } else {
    out.mQuantize.divisor = 0;
    out.mQuantize.stride = reader.read<u8>();
    reader.read<u8>();
  }
  auto err = ValidateQuantize(kind, out.mQuantize);
  if (!err.empty()) {
    return err;
  }
  out.mEntries.resize(reader.read<u16>());
  if constexpr (HasMinimum) {
    T minEnt, maxEnt;
    minEnt << reader;
    maxEnt << reader;

    out.mCachedMinMax = MinMax<T>{.min = minEnt, .max = maxEnt};
  }
  const auto nComponents =
      librii::gx::computeComponentCount(kind, out.mQuantize.mComp);

  reader.seekSet(start + startOfs);
  for (auto& entry : out.mEntries) {
    entry = librii::gx::readComponents<T>(reader, out.mQuantize.mType,
                                          nComponents, out.mQuantize.divisor);
  }

  if constexpr (HasMinimum) {
    // Unset if not needed
    if (out.mCachedMinMax.has_value() &&
        librii::g3d::ComputeMinMax(out) == *out.mCachedMinMax) {
      out.mCachedMinMax = std::nullopt;
    }
  }

  return ""; // Valid
}

// TODO: Move to own files

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

void ReadMesh(
    librii::g3d::PolygonData& poly, oishii::BinaryReader& reader, bool& isValid,

    const std::vector<librii::g3d::PositionBuffer>& positions,
    const std::vector<librii::g3d::NormalBuffer>& normals,
    const std::vector<librii::g3d::ColorBuffer>& colors,
    const std::vector<librii::g3d::TextureCoordinateBuffer>& texcoords,

    kpi::LightIOTransaction& transaction, const std::string& transaction_path) {
  const auto start = reader.tell();

  isValid &= reader.read<u32>() != 0; // size
  isValid &= reader.read<s32>() < 0;  // mdl offset

  poly.mCurrentMatrix = reader.read<s32>();
  reader.skip(12); // cache

  DlHandle primitiveSetup(reader);
  DlHandle primitiveData(reader);

  poly.mVertexDescriptor.mBitfield = reader.read<u32>();
  const u32 flag = reader.read<u32>();
  poly.currentMatrixEmbedded = flag & 1;
  if (poly.currentMatrixEmbedded) {
    // TODO (should be caught later)
  }
  poly.visible = !(flag & 2);

  poly.mName = readName(reader, start);
  poly.mId = reader.read<u32>();
  // TODO: Verify / cache
  isValid &= reader.read<u32>() > 0; // nVert
  isValid &= reader.read<u32>() > 0; // nPoly

  auto readBufHandle = [&](std::string& out, auto ifExist) {
    const auto hid = reader.read<s16>();
    if (hid < 0)
      out = "";
    else
      out = ifExist(hid);
  };

  readBufHandle(poly.mPositionBuffer,
                [&](s16 hid) { return positions[hid].mName; });
  readBufHandle(poly.mNormalBuffer,
                [&](s16 hid) { return normals[hid].mName; });
  for (int i = 0; i < 2; ++i) {
    readBufHandle(poly.mColorBuffer[i],
                  [&](s16 hid) { return colors[hid].mName; });
  }
  for (int i = 0; i < 8; ++i) {
    readBufHandle(poly.mTexCoordBuffer[i],
                  [&](s16 hid) { return texcoords[hid].mName; });
  }
  isValid &= reader.read<s32>() == -1; // fur
  reader.read<s32>();                  // matrix usage

  primitiveSetup.seekTo(reader);
  librii::gpu::QDisplayListVertexSetupHandler vcdHandler;
  librii::gpu::RunDisplayList(reader, vcdHandler, primitiveSetup.buf_size);

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
      assert(!"Unrecognized attribute");
    }
  }
  struct QDisplayListMeshHandler final
      : public librii::gpu::QDisplayListHandler {
    void onCommandDraw(oishii::BinaryReader& reader,
                       librii::gx::PrimitiveType type, u16 nverts,
                       u32 stream_end) override {
      if (mErr)
        return;

      assert(reader.tell() < stream_end);

      mLoadingMatrices = 0;
      mLoadingNrmMatrices = 0;
      mLoadingTexMatrices = 0;

      if (mPoly.mMatrixPrimitives.empty())
        mPoly.mMatrixPrimitives.push_back(librii::gx::MatrixPrimitive{});
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
              return;
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
    }
    void onCommandIndexedLoad(u32 cmd, u32 index, u16 address,
                              u8 size) override {
      // Expect IDX_A (position matrix) commands to be in sequence then IDX_B
      // (normal matrix) commands to follow.
      if (cmd == 0x20) {
        assert(address == mLoadingMatrices * 12);
        assert(size == 12);
        ++mLoadingMatrices;
      } else if (cmd == 0x28) {
        assert(address == 1024 + mLoadingNrmMatrices * 9);
        assert(size == 9);
        ++mLoadingNrmMatrices;
      } else if (cmd == 0x30) {
        assert(address == 120 + mLoadingTexMatrices * 12);
        assert(size == 12);
        ++mLoadingTexMatrices;
      } else {
        assert(!"Unknown matrix command");
      }

      // NOTE: Normal-matrix-only models would be ignored currently
      if (cmd != 0x20) {
        return;
      }
      if (mLoadingMatrices == 1) {
        mPoly.mMatrixPrimitives.emplace_back();
      }
      auto& mp = mPoly.mMatrixPrimitives.back();

      mp.mDrawMatrixIndices.push_back(index);
      // This is a J3D-only field, but we can fill it out anyway
      mp.mCurrentMatrix = mPoly.mCurrentMatrix;
    }
    QDisplayListMeshHandler(librii::g3d::PolygonData& poly) : mPoly(poly) {}
    bool mErr = false;
    int mLoadingMatrices = 0;
    int mLoadingNrmMatrices = 0;
    int mLoadingTexMatrices = 0;
    librii::g3d::PolygonData& mPoly;
  } meshHandler(poly);
  primitiveData.seekTo(reader);
  librii::gpu::RunDisplayList(reader, meshHandler, primitiveData.buf_size);
  if (meshHandler.mErr) {
    transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                         "Mesh unsupported.");
    transaction.state = kpi::TransactionState::Failure;
  }
}

void BinaryModel::read(oishii::BinaryReader& reader,
                       kpi::LightIOTransaction& transaction,
                       const std::string& transaction_path, bool& isValid) {
  const auto start = reader.tell();

  if (!reader.expectMagic<'MDL0', false>()) {
    transaction.state = kpi::TransactionState::Failure;
    return;
  }

  MAYBE_UNUSED const u32 fileSize = reader.read<u32>();
  const u32 revision = reader.read<u32>();
  if (revision != 11) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "MDL0 is version " + std::to_string(revision) +
                             ". Only MDL0 version 11 is supported.");
    transaction.state = kpi::TransactionState::Failure;
    return;
  }

  reader.read<s32>(); // ofsBRRES

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
    ofs = reader.read<s32>();

  name = readName(reader, start);
  info.read(reader);

  auto readDict = [&](u32 xofs, auto handler) {
    if (xofs) {
      reader.seekSet(start + xofs);
      librii::g3d::Dictionary _dict(reader);
      for (std::size_t i = 1; i < _dict.mNodes.size(); ++i) {
        const auto& dnode = _dict.mNodes[i];
        assert(dnode.mDataDestination);
        reader.seekSet(dnode.mDataDestination);
        handler(dnode);
      }
    }
  };

  {
    u32 bone_id = 0;
    readDict(secOfs.ofsBones, [&](const librii::g3d::DictionaryNode& dnode) {
      auto& bone = bones.emplace_back();
      if (!librii::g3d::readBone(bone, reader, bone_id,
                                 dnode.mDataDestination)) {
        printf("Failed to read bone %s\n", dnode.mName.c_str());
      }
      bone_id++;
    });
  }
  // Compute children
  for (int i = 0; i < bones.size(); ++i) {
    if (const auto parent_id = bones[i].mParent; parent_id >= 0) {
      if (parent_id >= bones.size()) {
        printf("Invalidly large parent index..\n");
        break;
      }
      bones[parent_id].mChildren.push_back(i);
    }
  }

  // Read Vertex data
  readDict(secOfs.ofsBuffers.position,
           [&](const librii::g3d::DictionaryNode& dnode) {
             auto err = readGenericBuffer(positions.emplace_back(), reader);
             if (err.size()) {
               transaction.callback(kpi::IOMessageClass::Error,
                                    transaction_path, err);
               transaction.state = kpi::TransactionState::Failure;
             }
           });
  readDict(secOfs.ofsBuffers.normal,
           [&](const librii::g3d::DictionaryNode& dnode) {
             auto err = readGenericBuffer(normals.emplace_back(), reader);
             if (err.size()) {
               transaction.callback(kpi::IOMessageClass::Error,
                                    transaction_path, err);
               transaction.state = kpi::TransactionState::Failure;
             }
           });
  readDict(secOfs.ofsBuffers.color,
           [&](const librii::g3d::DictionaryNode& dnode) {
             auto err = readGenericBuffer(colors.emplace_back(), reader);
             if (err.size()) {
               transaction.callback(kpi::IOMessageClass::Error,
                                    transaction_path, err);
               transaction.state = kpi::TransactionState::Failure;
             }
           });
  readDict(secOfs.ofsBuffers.uv, [&](const librii::g3d::DictionaryNode& dnode) {
    auto err = readGenericBuffer(texcoords.emplace_back(), reader);
    if (err.size()) {
      transaction.callback(kpi::IOMessageClass::Error, transaction_path, err);
      transaction.state = kpi::TransactionState::Failure;
    }
  });

  if (transaction.state == kpi::TransactionState::Failure)
    return;

  // TODO: Fur

  readDict(secOfs.ofsMaterials, [&](const librii::g3d::DictionaryNode& dnode) {
    auto& mat = materials.emplace_back();
    const bool ok = readMaterial(mat, reader);

    if (!ok) {
      printf("Failed to read material %s\n", dnode.mName.c_str());
    }
  });
  readDict(secOfs.ofsMeshes, [&](const librii::g3d::DictionaryNode& dnode) {
    auto& poly = meshes.emplace_back();
    ReadMesh(poly, reader, isValid, positions, normals, colors, texcoords,
             transaction, transaction_path);
  });

  if (transaction.state == kpi::TransactionState::Failure)
    return;

  readDict(secOfs.ofsRenderTree, [&](const librii::g3d::DictionaryNode& dnode) {
    auto commands = ByteCodeLists::ParseStream(reader);
    ByteCodeMethod c{dnode.mName, commands};
    bytecodes.emplace_back(c);
  });

  if (!isValid && bones.size() > 1) {
    transaction.callback(
        kpi::IOMessageClass::Error, transaction_path,
        "BRRES file was created with BrawlBox and is invalid. It is "
        "recommended you create BRRES files here by dropping a DAE/FBX file.");
    //
  } else if (!isValid) {
    transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                         "Note: BRRES file was saved with BrawlBox. Certain "
                         "materials may flicker during ghost replays.");
  } else if (bones.size() > 1) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "Rigging support is not fully tested.");
  }
}

} // namespace librii::g3d
