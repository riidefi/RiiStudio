#include "Common.hpp"
#include <core/common.h>
#include <librii/g3d/io/BoneIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

namespace riistudio::g3d {

struct RenderList {
  struct DrawCmd {
    u16 matIdx;
    u16 polyIdx;
    u16 boneIdx;
    u8 prio;

    RenderCommand type = RenderCommand::Draw;

    void write(oishii::Writer& writer) const {
      writer.writeUnaligned<u8>(static_cast<u8>(type));
      if (type == RenderCommand::Draw) {
        writer.writeUnaligned<u16>(matIdx);
        writer.writeUnaligned<u16>(polyIdx);
        writer.writeUnaligned<u16>(boneIdx);
        writer.writeUnaligned<u8>(prio);
      } else if (type == RenderCommand::NodeDescendence) {
        // Hack: Only for single-bone setups
        writer.writeUnaligned<u32>(0); // 0 : 0
      }
    }
  };
  std::vector<DrawCmd> cmds;
  std::string name;
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
    T min;
    T max;
    for (int c = 0; c < T::length(); ++c) {
      min[c] = +1'000'000'000.0f;
      max[c] = -1'000'000'000.0f;
    }
    for (auto& elem : buf.mEntries) {
      for (int c = 0; c < elem.length(); ++c) {
        min[c] = std::min(elem[c], min[c]);
        max[c] = std::max(elem[c], max[c]);
      }
    }

    min >> writer;
    max >> writer;
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

void writeVertexDataDL(const libcube::IndexedPolygon& poly,
                       const MatrixPrimitive& mp, oishii::Writer& writer) {
  using VATAttrib = librii::gx::VertexAttribute;
  using VATType = librii::gx::VertexAttributeType;

  for (auto& prim : mp.mPrimitives) {
    writer.write<u8>(librii::gx::EncodeDrawPrimitiveCommand(prim.mType));
    writer.write<u16>(prim.mVertices.size());
    for (const auto& v : prim.mVertices) {
      for (int a = 0; a < (int)VATAttrib::Max; ++a) {
        if (poly.getVcd()[(VATAttrib)a]) {
          switch (poly.getVcd().mAttributes.at((VATAttrib)a)) {
          case VATType::None:
            break;
          case VATType::Byte:
            writer.write<u8>(v[(VATAttrib)a]);
            break;
          case VATType::Short:
            writer.write<u16>(v[(VATAttrib)a]);
            break;
          case VATType::Direct:
            if (((VATAttrib)a) != VATAttrib::PositionNormalMatrixIndex) {
              assert(!"Direct vertex data is unsupported.");
              abort();
            }
            writer.write<u8>(v[(VATAttrib)a]);
            break;
          default:
            assert("!Unknown vertex attribute format.");
            abort();
          }
        }
      }
    }
  }

  // DL pad
  while (writer.tell() % 32)
    writer.write<u8>(0);
}

void WriteRenderList(const riistudio::g3d::RenderList& list,
                     oishii::Writer& writer) {
  for (auto& cmd : list.cmds)
    cmd.write(writer);
  writer.writeUnaligned<u8>(1); // terminator
}

void WriteShader(RelocWriter& linker, oishii::Writer& writer,
                 const librii::g3d::G3dShader& shader, std::size_t shader_start,
                 int shader_id) {
  DebugReport("Shader at %x\n", (unsigned)shader_start);
  linker.label("Shader" + std::to_string(shader_id), shader_start);

  librii::g3d::WriteTevBody(writer, shader_id, shader);
}

void WriteMesh(oishii::Writer& writer, const riistudio::g3d::Polygon& mesh,
               const riistudio::g3d::Model& mdl, const size_t& mesh_start,
               riistudio::g3d::NameTable& names) {
  const auto build_dlcache = [&]() {
    librii::gpu::DLBuilder dl(writer);
    writer.skip(5 * 2); // runtime
    dl.setVtxDescv(mesh.mVertexDescriptor);
    writer.write<u8>(0);
    // dl.align(); // 1
  };
  const auto build_dlsetup = [&]() {
    build_dlcache();
    librii::gpu::DLBuilder dl(writer);

    // Build desc
    // ----
    std::vector<
        std::pair<librii::gx::VertexAttribute, librii::gx::VQuantization>>
        desc;
    for (auto [attr, type] : mesh.getVcd().mAttributes) {
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
        const auto* buf = mdl.getBuf_Pos().findByName(mesh.mPositionBuffer);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color0: {
        const auto* buf = mdl.getBuf_Clr().findByName(mesh.mColorBuffer[0]);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color1: {
        const auto* buf = mdl.getBuf_Clr().findByName(mesh.mColorBuffer[1]);
        assert(buf);
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

        const auto* buf =
            mdl.getBuf_Uv().findByName(mesh.mTexCoordBuffer[chan]);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Normal:
      case VA::NormalBinormalTangent: {
        const auto* buf = mdl.getBuf_Nrm().findByName(mesh.mNormalBuffer);
        assert(buf);
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
    writer.skip(12 * 12); // array pointers set on runtime
    dl.align();           // 30
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
                    (static_cast<u32>(!mesh.isVisible()) << 1));
  writeNameForward(names, writer, mesh_start, mesh.getName());
  writer.write<u32>(mesh.mId);
  // TODO
  assert_since(0x40);
  const auto [nvtx, ntri] = librii::gx::ComputeVertTriCounts(mesh);
  writer.write<u32>(nvtx);
  writer.write<u32>(ntri);

  assert_since(0x48);
  const auto pos_idx = mdl.getBuf_Pos().indexOf(mesh.mPositionBuffer);
  writer.write<s16>(pos_idx);
  writer.write<s16>(mdl.getBuf_Nrm().indexOf(mesh.mNormalBuffer));
  for (int i = 0; i < 2; ++i)
    writer.write<s16>(mdl.getBuf_Clr().indexOf(mesh.mColorBuffer[i]));
  for (int i = 0; i < 8; ++i)
    writer.write<s16>(mdl.getBuf_Uv().indexOf(mesh.mTexCoordBuffer[i]));
  writer.write<s16>(-1); // fur
  writer.write<s16>(-1); // fur
  assert_since(0x64);
  writer.write<s32>(0x68); // msu, directly follows
  // TODO
  writer.write<u32>(0);
  writer.write<u32>(0);

  writer.alignTo(32);
  setup.setBufAddr(writer.tell());
  setup.setCmdSize(0xa0);
  setup.setBufSize(0xe0); // 0xa0 is already 32b aligned
  setup.write();
  build_dlsetup();

  writer.alignTo(32);
  data.setBufAddr(writer.tell());
  const auto data_start = writer.tell();
  {
    for (auto& mp : mesh.mMatrixPrimitives)
      writeVertexDataDL(mesh, mp, writer);
  }
  data.setCmdSize(writer.tell() - data_start);
  writer.alignTo(32);
  data.write();
}
void BuildRenderLists(const riistudio::g3d::Model& mdl,
                      std::vector<riistudio::g3d::RenderList>& renderLists) {
  RenderList nodeTree, drawOpa, drawXlu;
  nodeTree.name = "NodeTree";
  RenderList::DrawCmd root_bone_desc;
  root_bone_desc.type = RenderCommand::NodeDescendence;
  nodeTree.cmds.push_back(root_bone_desc);
  drawOpa.name = "DrawOpa";
  drawXlu.name = "DrawXlu";

  // Only supports one bone
  for (auto& draw : mdl.getBones()[0].mDisplayCommands) {
    RenderList::DrawCmd cmd;
    cmd.matIdx = draw.mMaterial;
    cmd.polyIdx = draw.mPoly;
    cmd.boneIdx = 0;
    cmd.prio = draw.mPrio;
    if (mdl.getMaterials()[draw.mMaterial].isXluPass())
      drawXlu.cmds.push_back(cmd);
    else
      drawOpa.cmds.push_back(cmd);
  }
  renderLists.push_back(nodeTree);
  if (!drawOpa.cmds.empty())
    renderLists.push_back(drawOpa);
  if (!drawXlu.cmds.empty())
    renderLists.push_back(drawXlu);
}
void writeModel(const Model& mdl, oishii::Writer& writer, RelocWriter& linker,
                NameTable& names, std::size_t brres_start) {
  const auto mdl_start = writer.tell();
  int d_cursor = 0;

  //
  // Build render lists
  //
  std::vector<RenderList> renderLists;
  BuildRenderLists(mdl, renderLists);

  //
  // Build shaders
  //

  librii::g3d::ShaderAllocator shader_allocator;
  for (auto& mat : mdl.getMaterials()) {
    librii::g3d::G3dShader shader(mat.getMaterialData());
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
  const auto write_dict_mat = [&](const std::string& name, auto src_range,
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
    writeNameForward(names, writer, mdl_start, mdl.mName, true);
  }
  {
    linker.label("MDL0_INFO");
    writer.write<u32>(0x40); // Header size
    linker.writeReloc<s32>("MDL0_INFO", "MDL0");
    writer.write<u32>(static_cast<u32>(mdl.mScalingRule));
    writer.write<u32>(static_cast<u32>(mdl.mTexMtxMode));

    const auto [nVert, nTri] = computeVertTriCounts(mdl);
    writer.write<u32>(nVert);
    writer.write<u32>(nTri);

    writer.write<u32>(0); // mdl.sourceLocation

    // TODO Bad assumption, nViewMtx
    writer.write<u32>(mdl.getBones().size());
    // TODO bMtxArray, bTexMtxArray, bBoundVolume
    writer.write<u8>(1);
    writer.write<u8>(0);
    writer.write<u8>(0);

    writer.write<u8>(static_cast<u8>(mdl.mEvpMtxMode));
    writer.write<u32>(0x40); // offset to bone table, effectively header size
    mdl.aabb.min >> writer;
    mdl.aabb.max >> writer;
    // Bone table
    // TODO: -1 for non-singlebound? Out of current scope for now
    writer.write<u32>(mdl.getBones().size());
    for (int i = 0; i < mdl.getBones().size(); ++i)
      writer.write<u32>(i);
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
  tally_dict("RenderTree", renderLists);
  tally_dict("Bones", mdl.getBones());
  tally_dict("Buffer_Position", mdl.getBuf_Pos());
  tally_dict("Buffer_Normal", mdl.getBuf_Nrm());
  tally_dict("Buffer_Color", mdl.getBuf_Clr());
  tally_dict("Buffer_UV", mdl.getBuf_Uv());
  tally_dict("Materials", mdl.getMaterials());
  tally_dict("Shaders", mdl.getMaterials());
  tally_dict("Meshes", mdl.getMeshes());
  u32 n_samplers = 0;
  for (auto& mat : mdl.getMaterials())
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

  // for (auto& mat : mdl.getMaterials()) {
  //   for (int s = 0; s < mat.samplers.size(); ++s) {
  //     auto& samp = *mat.samplers[s];
  //     tex_sampler_mappings.add_entry(samp.mTexture);
  //   }
  // }
  // Matching order..
  for (auto& tex : dynamic_cast<const Collection*>(mdl.childOf)->getTextures())
    for (auto& mat : mdl.getMaterials())
      for (int s = 0; s < mat.samplers.size(); ++s)
        if (mat.samplers[s].mTexture == tex.getName())
          tex_sampler_mappings.add_entry(tex.getName(), &mat, s);

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
      "RenderTree", renderLists,
      [&](const RenderList& list, std::size_t) {
        WriteRenderList(list, writer);
      },
      true, 1);

  u32 bone_id = 0;
  write_dict("Bones", mdl.getBones(),
             [&](const Bone& bone, std::size_t bone_start) {
               DebugReport("Bone at %x\n", (unsigned)bone_start);
               librii::g3d::WriteBone(names, writer, bone_start, bone, bone_id);
             });

  u32 mat_idx = 0;
  write_dict_mat(
      "Materials", mdl.getMaterials(),
      [&](const Material& mat, std::size_t mat_start) {
        linker.label("Mat" + std::to_string(mat_start), mat_start);
        librii::g3d::WriteMaterial(mat_start, writer, names, mat, mat_idx++,
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
    _dict.mNodes.resize(mdl.getMaterials().size() + 1);

    for (std::size_t i = 0; i < shader_allocator.size(); ++i) {
      writer.alignTo(32); //
      for (int j = 0; j < shader_allocator.matToShaderMap.size(); ++j) {
        if (shader_allocator.matToShaderMap[j] == i) {
          _dict.mNodes[j + 1].setDataDestination(writer.tell());
          _dict.mNodes[j + 1].setName(
              mdl.getMaterials()[j].IGCMaterial::getName());
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
      "Meshes", mdl.getMeshes(),
      [&](const g3d::Polygon& mesh, std::size_t mesh_start) {
        WriteMesh(writer, mesh, mdl, mesh_start, names);
      },
      false, 32);

  write_dict(
      "Buffer_Position", mdl.getBuf_Pos(),
      [&](const PositionBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_Normal", mdl.getBuf_Nrm(),
      [&](const NormalBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_Color", mdl.getBuf_Clr(),
      [&](const ColorBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_UV", mdl.getBuf_Uv(),
      [&](const TextureCoordinateBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);

  linker.writeChildren();
  linker.label("MDL0_END");
  linker.resolve();
} // namespace riistudio::g3d

} // namespace riistudio::g3d
