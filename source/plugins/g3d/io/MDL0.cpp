#include "Common.hpp"
#include <core/common.h>
#include <librii/g3d/io/BoneIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/ModelIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

namespace riistudio::g3d {

struct RenderList : public librii::g3d::ByteCodeMethod {
  std::string getName() const { return name; }
};
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

  for (auto& entry : buf.mEntries) {
    librii::gx::writeComponents(writer, entry, buf.mQuantize.mType, nComponents,
                                buf.mQuantize.divisor);
  }
  writer.alignTo(32);
} // namespace riistudio::g3d

std::string writeVertexDataDL(const librii::g3d::PolygonData& poly,
                              const MatrixPrimitive& mp,
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
                 const librii::g3d::G3dShader& shader, std::size_t shader_start,
                 int shader_id) {
  DebugReport("Shader at %x\n", (unsigned)shader_start);
  linker.label("Shader" + std::to_string(shader_id), shader_start);

  librii::g3d::WriteTevBody(writer, shader_id, shader);
}

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

std::string WriteMesh(oishii::Writer& writer,
                      const librii::g3d::PolygonData& mesh,
                      const librii::g3d::BinaryModel& mdl,
                      const size_t& mesh_start,
                      riistudio::g3d::NameTable& names) {
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
void BuildRenderLists(const Model& mdl,
                      std::vector<librii::g3d::ByteCodeMethod>& renderLists) {
  librii::g3d::ByteCodeMethod nodeTree{.name = "NodeTree"};
  librii::g3d::ByteCodeMethod nodeMix{.name = "NodeMix"};
  librii::g3d::ByteCodeMethod drawOpa{.name = "DrawOpa"};
  librii::g3d::ByteCodeMethod drawXlu{.name = "DrawXlu"};

  for (size_t i = 0; i < mdl.getBones().size(); ++i) {
    for (const auto& draw : mdl.getBones()[i].mDisplayCommands) {
      librii::g3d::ByteCodeLists::Draw cmd{
          .matId = static_cast<u16>(draw.mMaterial),
          .polyId = static_cast<u16>(draw.mPoly),
          .boneId = static_cast<u16>(i),
          .prio = draw.mPrio,
      };
      bool xlu = draw.mMaterial < std::size(mdl.getMaterials()) &&
                 mdl.getMaterials()[draw.mMaterial].xlu;
      (xlu ? &drawXlu : &drawOpa)->commands.push_back(cmd);
    }
    auto parent = mdl.getBones()[i].mParent;
    assert(parent < std::ssize(mdl.getBones()));
    librii::g3d::ByteCodeLists::NodeDescendence desc{
        .boneId = static_cast<u16>(i),
        .parentMtxId =
            static_cast<u16>(parent >= 0 ? mdl.getBones()[parent].matrixId : 0),
    };
    nodeTree.commands.push_back(desc);
  }

  auto write_drw = [&](const libcube::DrawMatrix& drw, size_t i) {
    if (drw.mWeights.size() > 1) {
      librii::g3d::ByteCodeLists::NodeMix mix{
          .mtxId = static_cast<u16>(i),
      };
      for (auto& weight : drw.mWeights) {
        assert(weight.boneId < mdl.getBones().size());
        mix.blendMatrices.push_back(
            librii::g3d::ByteCodeLists::NodeMix::BlendMtx{
                .mtxId =
                    static_cast<u16>(mdl.getBones()[weight.boneId].matrixId),
                .ratio = weight.weight,
            });
      }
      nodeMix.commands.push_back(mix);
    } else {
      assert(drw.mWeights[0].boneId < mdl.getBones().size());
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
  for (auto& mtx : mdl.mDrawMatrices) {
    if (mtx.mWeights.size() > 1) {
      needs_nodemix = true;
      break;
    }
  }

  if (needs_nodemix) {
    // Bones come first
    for (auto& bone : mdl.getBones()) {
      auto& drw = mdl.mDrawMatrices[bone.matrixId];
      write_drw(drw, bone.matrixId);
    }
    for (size_t i = 0; i < mdl.mDrawMatrices.size(); ++i) {
      auto& drw = mdl.mDrawMatrices[i];
      if (drw.mWeights.size() == 1) {
        // Written in pre-pass
        continue;
      }
      write_drw(drw, i);
    }
  }

  renderLists.push_back(RenderList{nodeTree});
  if (!nodeMix.commands.empty()) {
    renderLists.push_back(RenderList{nodeMix});
  }
  if (!drawOpa.commands.empty()) {
    renderLists.push_back(RenderList{drawOpa});
  }
  if (!drawXlu.commands.empty()) {
    renderLists.push_back(RenderList{drawXlu});
  }
}

void writeModel(librii::g3d::BinaryModel& bin, oishii::Writer& writer,
                RelocWriter& linker, NameTable& names, std::size_t brres_start,
                std::span<const librii::g3d::TextureData> textures) {
  const auto mdl_start = writer.tell();
  int d_cursor = 0;
  //
  // Build shaders
  //

  librii::g3d::ShaderAllocator shader_allocator;
  for (auto& mat : bin.materials) {
    librii::g3d::G3dShader shader(mat);
    shader_allocator.alloc(shader);
  }

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
  u32 n_samplers = 0;
  for (auto& mat : bin.materials)
    n_samplers += mat.samplers.size();
  if (n_samplers) {
    Dictionaries.emplace("TexSamplerMap", dicts_size + d_cursor);
    dicts_size += 24 + 16 * n_samplers;
  }
  for (auto [key, val] : Dictionaries) {
    DebugReport("%s: %x\n", key.c_str(), (unsigned)val);
  }
  writer.skip(dicts_size);

  librii::g3d::TextureSamplerMappingManager tex_sampler_mappings;

  //
  // NOTE: A raw ref to materials is taken here
  //
  // A spurious copy of the `materials` array was breaking it!
  //
  // #define MATERIALS (mdl.getMaterials())
  // #define MATERIALS bin.materials

  // for (auto& mat : mdl.getMaterials()) {
  //   for (int s = 0; s < mat.samplers.size(); ++s) {
  //     auto& samp = *mat.samplers[s];
  //     tex_sampler_mappings.add_entry(samp.mTexture);
  //   }
  // }
  // Matching order..
  for (auto& tex : textures)
    for (auto& mat : bin.materials)
      for (int s = 0; s < mat.samplers.size(); ++s)
        if (mat.samplers[s].mTexture == tex.name)
          tex_sampler_mappings.add_entry(tex.name, &mat, s);

  int sm_i = 0;
  write_dict(
      "TexSamplerMap", tex_sampler_mappings,
      [&](librii::g3d::TextureSamplerMapping& map, std::size_t start) {
        writer.write<u32>(map.entries.size());
        for (int i = 0; i < map.entries.size(); ++i) {
          tex_sampler_mappings.entries[sm_i].entries[i] = writer.tell();
          writer.write<s32>(0);
          writer.write<s32>(0);
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

  std::set<s16> displayMatrices =
      librii::gx::computeDisplayMatricesSubset(bin.meshes);
  u32 bone_id = 0;
  write_dict("Bones", bin.bones,
             [&](const librii::g3d::BoneData& bone, std::size_t bone_start) {
               DebugReport("Bone at %x\n", (unsigned)bone_start);
               librii::g3d::WriteBone(names, writer, bone_start, bone,
                                      bin.bones, bone_id++, displayMatrices);
             });

  u32 mat_idx = 0;
  write_dict_mat(
      "Materials", bin.materials,
      [&](const librii::g3d::G3dMaterialData& mat, std::size_t mat_start) {
        linker.label("Mat" + std::to_string(mat_start), mat_start);
        librii::g3d::WriteMaterialBody(mat_start, writer, names, mat, mat_idx++,
                                       linker, shader_allocator,
                                       tex_sampler_mappings);
      },
      false, 4);

  // Shaders
  {
    if (shader_allocator.size() == 0)
      return;

    u32 dict_ofs = Dictionaries.at("Shaders");
    librii::g3d::QDictionary _dict;
    _dict.mNodes.resize(bin.materials.size() + 1);

    for (std::size_t i = 0; i < shader_allocator.size(); ++i) {
      writer.alignTo(32); //
      for (int j = 0; j < shader_allocator.matToShaderMap.size(); ++j) {
        if (shader_allocator.matToShaderMap[j] == i) {
          _dict.mNodes[j + 1].setDataDestination(writer.tell());
          _dict.mNodes[j + 1].setName(bin.materials[j].name);
        }
      }
      const auto backpatch = writePlaceholder(writer); // size
      writer.write<s32>(mdl_start - backpatch);        // mdl0 offset
      WriteShader(linker, writer, shader_allocator.shaders[i], backpatch, i);
      writeOffsetBackpatch(writer, backpatch, backpatch);
    }
    {
      oishii::Jump<oishii::Whence::Set, oishii::Writer> d(writer, dict_ofs);
      linker.label("Shaders");
      _dict.write(writer, names);
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
void writeModel(const Model& mdl, oishii::Writer& writer, RelocWriter& linker,
                NameTable& names, std::size_t brres_start) {
  librii::g3d::BinaryModel bin;

  bin.bones = {mdl.getBones().begin(), mdl.getBones().end()};
  bin.materials = {mdl.getMaterials().begin(), mdl.getMaterials().end()};
  bin.meshes = {mdl.getMeshes().begin(), mdl.getMeshes().end()};
  bin.positions = {mdl.getBuf_Pos().begin(), mdl.getBuf_Pos().end()};
  bin.normals = {mdl.getBuf_Nrm().begin(), mdl.getBuf_Nrm().end()};
  bin.colors = {mdl.getBuf_Clr().begin(), mdl.getBuf_Clr().end()};
  bin.texcoords = {mdl.getBuf_Uv().begin(), mdl.getBuf_Uv().end()};
  // Compute ModelInfo
  {
    std::set<s16> displayMatrices =
        librii::gx::computeDisplayMatricesSubset(mdl.getMeshes());
    const auto [nVert, nTri] = computeVertTriCounts(mdl);
    bool nrmMtx = std::any_of(mdl.getMeshes().begin(), mdl.getMeshes().end(),
                              [](auto& m) { return m.needsNormalMtx(); });
    bool texMtx = std::any_of(mdl.getMeshes().begin(), mdl.getMeshes().end(),
                              [](auto& m) { return m.needsTextureMtx(); });
    librii::g3d::BinaryModelInfo info{
        .scalingRule = mdl.mScalingRule,
        .texMtxMode = mdl.mTexMtxMode,
        .numVerts = nVert,
        .numTris = nTri,
        .sourceLocation = mdl.sourceLocation,
        .numViewMtx = static_cast<u32>(displayMatrices.size()),
        .normalMtxArray = nrmMtx,
        .texMtxArray = texMtx,
        .boundVolume = false,
        .evpMtxMode = mdl.mEvpMtxMode,
        .min = mdl.aabb.min,
        .max = mdl.aabb.max,
    };
    {
      // Matrix -> Bone LUT
      auto& lut = info.mtxToBoneLUT.mtxIdToBoneId;
      lut.resize(mdl.mDrawMatrices.size());
      for (size_t i = 0; i < mdl.mDrawMatrices.size(); ++i) {
        auto& mtx = mdl.mDrawMatrices[i];
        bool multi_influence = mtx.mWeights.size() > 1;
        s32 boneId = -1;
        if (!multi_influence) {
          auto it =
              std::find_if(mdl.getBones().begin(), mdl.getBones().end(),
                           [i](auto& bone) { return bone.matrixId == i; });
          if (it != mdl.getBones().end()) {
            boneId = it - mdl.getBones().begin();
          }
        }
        lut[i] = boneId;
      }
    }
    bin.info = info;
  }
  bin.name = mdl.mName;
  BuildRenderLists(mdl, bin.bytecodes);

  auto texRange = dynamic_cast<const Collection*>(mdl.childOf)->getTextures();
  std::vector<librii::g3d::TextureData> textures(texRange.begin(),
                                                 texRange.end());
  writeModel(bin, writer, linker, names, brres_start, textures);
}

} // namespace riistudio::g3d
