#include <core/common.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

#include "Common.hpp"
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>
#include <plugins/g3d/util/Dictionary.hpp>

namespace riistudio::g3d {

enum class RenderCommand {
  NoOp,
  Return,

  NodeDescendence,
  NodeMixing,

  Draw,

  EnvelopeMatrix,
  MatrixCopy
};

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
template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
void readGenericBuffer(GenericBuffer<T, HasMinimum, HasDivisor, kind>& out,
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
  out.mEntries.resize(reader.read<u16>());
  T minEnt, maxEnt;
  if (HasMinimum) {
    minEnt << reader;
    maxEnt << reader;
  }
  const auto nComponents =
      librii::gx::computeComponentCount(kind, out.mQuantize.mComp);
  assert((kind != librii::gx::VertexBufferKind::normal) ||
         (!((int)out.mQuantize.divisor != 6 &&
            out.mQuantize.mType.generic ==
                librii::gx::VertexBufferType::Generic::s8) &&
          !((int)out.mQuantize.divisor != 14 &&
            out.mQuantize.mType.generic ==
                librii::gx::VertexBufferType::Generic::s16)));
  assert(kind != librii::gx::VertexBufferKind::normal ||
         (out.mQuantize.mType.generic !=
              librii::gx::VertexBufferType::Generic::u8 &&
          out.mQuantize.mType.generic !=
              librii::gx::VertexBufferType::Generic::u16));

  reader.seekSet(start + startOfs);
  // TODO: Recompute bounds
  for (auto& entry : out.mEntries) {
    entry = librii::gx::readComponents<T>(reader, out.mQuantize.mType,
                                          nComponents, out.mQuantize.divisor);
  }
}

// Does not write size or mdl0 offset
template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
void writeGenericBuffer(
    const GenericBuffer<T, HasMinimum, HasDivisor, kind>& buf,
    oishii::Writer& writer, u32 header_start, NameTable& names) {
  const auto backpatch_array_ofs = writePlaceholder(writer);
  writeNameForward(names, writer, header_start, buf.getName());
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
static void writeMaterialDisplayList(const libcube::GCMaterialData& mat,
                                     oishii::Writer& writer) {
  MAYBE_UNUSED const auto dl_start = writer.tell();
  DebugReport("Mat dl start: %x\n", (unsigned)writer.tell());
  librii::gpu::DLBuilder dl(writer);
  {
    dl.setAlphaCompare(mat.alphaCompare);
    dl.setZMode(mat.zMode);
    dl.setBlendMode(mat.blendMode);
    dl.setDstAlpha(false, 0); // TODO
    dl.align();
  }
  assert(writer.tell() - dl_start == 0x20);
  {
    for (int i = 0; i < 3; ++i)
      dl.setTevColor(i + 1, mat.tevColors[i + 1]);
    dl.align(); // pad 4
    for (int i = 0; i < 4; ++i)
      dl.setTevKColor(i, mat.tevKonstColors[i]);
    dl.align(); // 24
  }
  assert(writer.tell() - dl_start == 0xa0);
  {
    std::array<librii::gx::IndirectTextureScalePair, 4> scales;
    for (int i = 0; i < mat.indirectStages.size(); ++i)
      scales[i] = mat.indirectStages[i].scale;
    dl.setIndTexCoordScale(0, scales[0], scales[1]);
    dl.setIndTexCoordScale(2, scales[2], scales[3]);

    const auto write_ind_mtx = [&](std::size_t i) {
      if (mat.mIndMatrices.size() > i)
        dl.setIndTexMtx(i, mat.mIndMatrices[i]);
      else
        writer.skip(5 * 3);
    };
    write_ind_mtx(0);
    dl.align(); // 7
    write_ind_mtx(1);
    write_ind_mtx(2);
    dl.align();
  }
  assert(writer.tell() - dl_start == 0xe0);
  {
    for (int i = 0; i < mat.texGens.size(); ++i)
      dl.setTexCoordGen(i, mat.texGens[i]);
    writer.skip(18 * (8 - mat.texGens.size()));
    dl.align();
  }
  assert(writer.tell() - dl_start == 0x180);
}

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
void writeModel(const Model& mdl, oishii::Writer& writer, RelocWriter& linker,
                NameTable& names, std::size_t brres_start) {
  const auto mdl_start = writer.tell();
  int d_cursor = 0;

  //
  // Build render lists
  //
  std::vector<RenderList> renderLists;
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

  //
  // Build shaders
  //
  struct G3dShader {
    bool operator==(const G3dShader&) const = default;

    // Fixed-size DL
    librii::gx::SwapTable mSwapTable;
    rsl::array_vector<librii::gx::IndOrder, 4> mIndirectOrders;

    // Variable-sized DL
    rsl::array_vector<librii::gx::TevStage, 16> mStages;

    G3dShader(const librii::gx::LowLevelGxMaterial& mat)
        : mSwapTable(mat.mSwapTable) {
      mIndirectOrders.resize(mat.indirectStages.size());
      for (int i = 0; i < mIndirectOrders.size(); ++i)
        mIndirectOrders[i] = mat.indirectStages[i].order;

      mStages.resize(mat.mStages.size());
      for (int i = 0; i < mat.mStages.size(); ++i)
        mStages[i] = mat.mStages[i];
    }
  };
  std::vector<G3dShader> shaders;
  std::vector<u32> matToShaderMap;
  for (auto& mat : mdl.getMaterials()) {
    G3dShader shader(mat.getMaterialData());
    auto found = std::find(shaders.begin(), shaders.end(), shader);
    if (found == shaders.end()) {
      matToShaderMap.emplace_back(shaders.size());
      shaders.emplace_back(shader);
    } else {
      matToShaderMap.emplace_back(found - shaders.begin());
    }
  }
  const auto get_shader_id = [&](const G3dShader& shader) {
    const auto found = std::find(shaders.begin(), shaders.end(), shader);
    assert(found != shaders.end());
    return "Shader" + std::to_string(std::distance(shaders.begin(), found));
  };

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

  struct TextureSamplerMapping {
    TextureSamplerMapping() = default;
    TextureSamplerMapping(std::string n, u32 pos) : name(n) {
      entries.emplace_back(pos);
    }

    const std::string& getName() const { return name; }
    std::string name;
    llvm::SmallVector<u32, 1> entries; // stream position
  };
  struct TextureSamplerMappingManager {
    void add_entry(const std::string& name, const Material* mat,
                   int mat_sampl_index) {
      if (auto found = std::find_if(entries.begin(), entries.end(),
                                    [name](auto& e) { return e.name == name; });
          found != entries.end()) {
        matMap[{mat, mat_sampl_index}] = {found - entries.begin(),
                                          found->entries.size()};
        found->entries.emplace_back(0);
        return;
      }
      matMap[{mat, mat_sampl_index}] = {entries.size(), 0};
      entries.emplace_back(name, 0);
    }
    std::pair<u32, u32> from_mat(const Material* mat, int sampl_idx) {
      std::pair key{mat, sampl_idx};
      if (!matMap.contains(key)) {
        // TODO: Invalid tex ref.
        return {0, 0};
      }
      assert(matMap.contains(key));
      const auto [entry_n, sub_n] = matMap.at(key);
      const auto entry_ofs = entries[entry_n].entries[sub_n];
      return {entry_ofs, entry_ofs - sub_n * 8 - 4};
    }
    std::vector<TextureSamplerMapping> entries;
    std::map<std::pair<const Material*, int>, std::pair<int, int>> matMap;

    auto begin() { return entries.begin(); }
    auto end() { return entries.end(); }
    std::size_t size() const { return entries.size(); }
    TextureSamplerMapping& operator[](std::size_t i) { return entries[i]; }
    const TextureSamplerMapping& operator[](std::size_t i) const {
      return entries[i];
    }
  };

  TextureSamplerMappingManager tex_sampler_mappings;

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
      [&](TextureSamplerMapping& map, std::size_t start) {
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
        for (auto& cmd : list.cmds)
          cmd.write(writer);
        writer.writeUnaligned<u8>(1); // terminator
      },
      true, 1);

  u32 bone_id = 0;
  write_dict(
      "Bones", mdl.getBones(), [&](const Bone& bone, std::size_t bone_start) {
        DebugReport("Bone at %x\n", (unsigned)bone_start);
        writeNameForward(names, writer, bone_start, bone.getName());
        writer.write<u32>(bone_id++);
        writer.write<u32>(bone.matrixId);
        writer.write<u32>(computeFlag(bone));
        writer.write<u32>(bone.billboardType);
        writer.write<u32>(0); // TODO: ref
        bone.mScaling >> writer;
        bone.mRotation >> writer;
        bone.mTranslation >> writer;
        bone.mVolume.min >> writer;
        bone.mVolume.max >> writer;

        // Parent, Child, Left, Right
        writer.write<s32>(0);
        writer.write<s32>(0);
        writer.write<s32>(0);
        writer.write<s32>(0);

        writer.write<u32>(0); // user data

        // Recomputed on runtime?
        // Mtx + Inverse, 3x4 f32
        std::array<f32, 2 * 3 * 4> matrix_data{
            1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,
            1.0, -0.0, 0.0, 0.0, -0.0, 1.0, -0.0, 0.0, 0.0, -0.0, 1.0, 0.0};
        for (auto d : matrix_data)
          writer.write<f32>(d);
      });

  u32 mat_idx = 0;
  write_dict_mat(
      "Materials", mdl.getMaterials(),
      [&](const Material& mat, std::size_t mat_start) {
        linker.label("Mat" + std::to_string(mat_start), mat_start);
        DebugReport("MAT_START %x\n", (u32)mat_start);
        DebugReport("MAT_NAME %x\n", writer.tell());
        writeNameForward(names, writer, mat_start, mat.IGCMaterial::getName());
        DebugReport("MATIDAT %x\n", writer.tell());
        writer.write<u32>(mat_idx++);
        u32 flag = mat.flag;
        flag = (flag & ~0x80000000) | (mat.xlu ? 0x80000000 : 0);
        writer.write<u32>(flag);
        writer.write<u8>(static_cast<u8>(mat.texGens.size()));
        writer.write<u8>(static_cast<u8>(mat.chanData.size()));
        writer.write<u8>(static_cast<u8>(mat.mStages.size()));
        writer.write<u8>(static_cast<u8>(mat.indirectStages.size()));
        writer.write<u32>(static_cast<u32>(mat.cullMode));
        // Misc
        writer.write<u8>(mat.earlyZComparison);
        writer.write<u8>(mat.lightSetIndex);
        writer.write<u8>(mat.fogIndex);
        writer.write<u8>(0); // pad
        for (u8 i = 0; i < mat.indirectStages.size(); ++i)
          writer.write<u8>(static_cast<u8>(mat.indConfig[i].method));
        for (u8 i = mat.indirectStages.size(); i < 4; ++i)
          writer.write<u8>(0);
        for (u8 i = 0; i < mat.indirectStages.size(); ++i)
          writer.write<u8>(mat.indConfig[i].normalMapLightRef);
        for (u8 i = mat.indirectStages.size(); i < 4; ++i)
          writer.write<u8>(0xff);
        linker.writeReloc<s32>("Mat" + std::to_string(mat_start),
                               get_shader_id(mat));
        writer.write<u32>(mat.samplers.size());
        u32 sampler_offset = 0;
        if (mat.samplers.size()) {
          sampler_offset = writePlaceholder(writer);
        } else {
          writer.write<s32>(0); // no samplers
          printf(">> Mat %s has no samplers.\n", mat.name.c_str());
        }
        writer.write<s32>(0); // fur
        writer.write<s32>(0); // ud
        const auto dl_offset = writePlaceholder(writer);

        // Texture and palette objects are set on runtime.
        writer.skip((4 + 8 * 12) + (4 + 8 * 32));

        // Texture transformations

        const auto build_flags =
            [](const libcube::GCMaterialData::TexMatrix& mtx) -> u32 {
          // TODO: Float equality
          const u32 identity_scale = mtx.scale == glm::vec2{1.0f, 1.0f};
          const u32 identity_rotate = mtx.rotate == 0.0f;
          const u32 identity_translate = mtx.translate == glm::vec2{0.0f, 0.0f};
          // printf("identity_scale: %u, identity_rotate: %u,
          // identity_translate: "
          //        "%u\n",
          //        identity_scale, identity_rotate, identity_translate);
          return 1 | (identity_scale << 1) | (identity_rotate << 2) |
                 (identity_translate << 3);
        };
        u32 tex_flags = 0;
        for (int i = mat.texMatrices.size() - 1; i >= 0; --i) {
          tex_flags = (tex_flags << 4) | build_flags(mat.texMatrices[i]);
        }
        writer.write<u32>(tex_flags);
        // Unlike J3D, only one texmatrix mode per material
        using CommonTransformModel =
            libcube::GCMaterialData::CommonTransformModel;
        CommonTransformModel texmtx_mode = CommonTransformModel::Default;
        for (int i = 0; i < mat.texMatrices.size(); ++i) {
          texmtx_mode = mat.texMatrices[i].transformModel;
        }
        switch (texmtx_mode) {
        case CommonTransformModel::Default:
        case CommonTransformModel::Maya:
        default:
          writer.write<u32>(0);
          break;
        case CommonTransformModel::XSI:
          writer.write<u32>(1);
          break;
        case CommonTransformModel::Max:
          writer.write<u32>(2);
          break;
        }
        for (int i = 0; i < mat.texMatrices.size(); ++i) {
          auto& mtx = mat.texMatrices[i];

          writer.write<f32>(mtx.scale.x);
          writer.write<f32>(mtx.scale.y);
          writer.write<f32>(glm::degrees(mtx.rotate));
          writer.write<f32>(mtx.translate.x);
          writer.write<f32>(mtx.translate.y);
        }
        for (int i = mat.texMatrices.size(); i < 8; ++i) {
          writer.write<f32>(1.0f);
          writer.write<f32>(1.0f);
          writer.write<f32>(0.0f);
          writer.write<f32>(0.0f);
          writer.write<f32>(0.0f);
        }
        std::array<f32, 3 * 4> ident34{1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
                                       0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
        for (int i = 0; i < mat.texMatrices.size(); ++i) {
          auto& mtx = mat.texMatrices[i];

          // printf("CamIdx: %i, LIdx: %i\n", (signed)mtx.camIdx,
          //        (signed)mtx.lightIdx);
          writer.write<s8>((s8)mtx.camIdx);
          writer.write<s8>((s8)mtx.lightIdx);

          using CommonMappingMethod =
              libcube::GCMaterialData::CommonMappingMethod;
          switch (mtx.method) {
          case CommonMappingMethod::Standard:
          default:
            writer.write<u8>(0);
            break;
          case CommonMappingMethod::EnvironmentMapping:
          case CommonMappingMethod::
              ManualEnvironmentMapping: // In G3D, Light/Specular are used;
                                        // in J3D, it'd be to the game
            writer.write<u8>(1);
            break;
          case CommonMappingMethod::ViewProjectionMapping:
            writer.write<u8>(2);
            break;
            // EGG extension
          case CommonMappingMethod::ProjectionMapping:
            writer.write<u8>(5);
            break;
          case CommonMappingMethod::EnvironmentLightMapping:
            writer.write<u8>(3);
            break;
          case CommonMappingMethod::EnvironmentSpecularMapping:
            writer.write<u8>(4);
            break;
          }
          // No effect matrix support yet
          writer.write<u8>(1);

          for (auto d : ident34)
            writer.write<f32>(d);
        }
        for (int i = mat.texMatrices.size(); i < 8; ++i) {
          writer.write<u8>(0xff); // cam
          writer.write<u8>(0xff); // light
          writer.write<u8>(0);    // map
          writer.write<u8>(1);    // flag

          for (auto d : ident34)
            writer.write<f32>(d);
        }

        for (u8 i = 0; i < mat.chanData.size(); ++i) {
          // TODO: flag
          writer.write<u32>(0x3f);
          mat.chanData[i].matColor >> writer;
          mat.chanData[i].ambColor >> writer;

          librii::gpu::LitChannel clr, alpha;

          clr.from(mat.colorChanControls[i]);
          alpha.from(mat.colorChanControls[i + 1]);

          writer.write<u32>(clr.hex);
          writer.write<u32>(alpha.hex);
        }
        for (u8 i = mat.chanData.size(); i < 2; ++i) {
          writer.write<u32>(0x3f); // TODO: flag, reset on runtime
          writer.write<u32>(0);    // mclr
          writer.write<u32>(0);    // aclr
          writer.write<u32>(0);    // clr
          writer.write<u32>(0);    // alph
        }

        // Not written if no samplers..
        if (sampler_offset != 0) {
          writeOffsetBackpatch(writer, sampler_offset, mat_start);
          for (int i = 0; i < mat.samplers.size(); ++i) {
            const auto s_start = writer.tell();
            const auto& sampler = mat.samplers[i];

            {
              const auto [entry_start, struct_start] =
                  tex_sampler_mappings.from_mat(&mat, i);
              DebugReport("<material=\"%s\" sampler=%u>\n",
                          mat.IGCMaterial::getName().c_str(), i);
              DebugReport("\tentry_start=%x, struct_start=%x\n",
                          (unsigned)entry_start, (unsigned)struct_start);
              oishii::Jump<oishii::Whence::Set, oishii::Writer> sg(writer,
                                                                   entry_start);
              writer.write<s32>(mat_start - struct_start);
              writer.write<s32>(s_start - struct_start);
            }

            writeNameForward(names, writer, s_start, sampler.mTexture);
            writeNameForward(names, writer, s_start, sampler.mPalette);
            writer.skip(8);       // runtime pointers
            writer.write<u32>(i); // gpu texture slot
            writer.write<u32>(i); // no palette support
            writer.write<u32>(static_cast<u32>(sampler.mWrapU));
            writer.write<u32>(static_cast<u32>(sampler.mWrapV));
            writer.write<u32>(static_cast<u32>(sampler.mMinFilter));
            writer.write<u32>(static_cast<u32>(sampler.mMagFilter));
            writer.write<f32>(sampler.mLodBias);
            writer.write<u32>(static_cast<u32>(sampler.mMaxAniso));
            writer.write<u8>(sampler.bBiasClamp);
            writer.write<u8>(sampler.bEdgeLod);
            writer.skip(2);
          }
        }
        writer.alignTo(32);
        writeOffsetBackpatch(writer, dl_offset, mat_start);
        writeMaterialDisplayList(mat.getMaterialData(), writer);
      },
      false, 4);

  const auto write_shader = [&](const G3dShader& shader,
                                std::size_t shader_start, int shader_id) {
    DebugReport("Shader at %x\n", (unsigned)shader_start);
    linker.label("Shader" + std::to_string(shader_id), shader_start);
    writer.write<u32>(shader_id);
    writer.write<u8>(shader.mStages.size());
    writer.skip(3);
    // texgens and maps are coupled
    std::array<u8, 8> genTexMapping;
    std::fill(genTexMapping.begin(), genTexMapping.end(), 0xff);
    for (auto& stage : shader.mStages)
      if (stage.texCoord != 0xff && stage.texMap != 0xff)
        genTexMapping[stage.texCoord] = stage.texMap;
    for (auto& order : shader.mIndirectOrders)
      if (order.refCoord != 0xff && order.refMap != 0xff)
        genTexMapping[order.refCoord] = order.refMap;
    for (auto e : genTexMapping)
      writer.write<u8>(e);
    writer.skip(8);

    // DL
    assert(writer.tell() - shader_start == 32);
    librii::gpu::DLBuilder dl(writer);
    for (int i = 0; i < 4; ++i)
      dl.setTevSwapModeTable(i, shader.mSwapTable[i]);
    auto ind_orders = shader.mIndirectOrders;
    ind_orders.resize(4);
    dl.setIndTexOrder(ind_orders[0].refCoord, ind_orders[0].refMap,
                      ind_orders[1].refCoord, ind_orders[1].refMap,
                      ind_orders[2].refCoord, ind_orders[2].refMap,
                      ind_orders[3].refCoord, ind_orders[3].refMap);
    dl.align(); // 11

    assert(writer.tell() - shader_start == 32 + 0x60);

    std::array<librii::gx::TevStage, 16> stages;
    for (int i = 0; i < shader.mStages.size(); ++i)
      stages[i] = shader.mStages[i];

    const auto stages_count = shader.mStages.size();
    const auto stages_count_rounded = roundUp(stages_count, 2);

    for (unsigned i = 0; i < stages_count_rounded; i += 2) {
      const auto& even_stage = stages[i];
      const auto& odd_stage = stages[i + 1];

      const auto couple_start = writer.tell();

      dl.setTevKonstantSel(
          i, even_stage.colorStage.constantSelection,
          even_stage.alphaStage.constantSelection,
          i + 1 < stages_count ? odd_stage.colorStage.constantSelection
                               : librii::gx::TevKColorSel::const_8_8,
          i + 1 < stages_count ? odd_stage.alphaStage.constantSelection
                               : librii::gx::TevKAlphaSel::const_8_8);

      dl.setTevOrder(i, even_stage, odd_stage);

      dl.setTevColorCalc(i, even_stage.colorStage);
      if (i + 1 < stages_count)
        dl.setTevColorCalc(i + 1, odd_stage.colorStage);
      else
        writer.skip(5);

      dl.setTevAlphaCalcAndSwap(i, even_stage.alphaStage, even_stage.rasSwap,
                                even_stage.texMapSwap);
      if (i + 1 < stages_count)
        dl.setTevAlphaCalcAndSwap(i + 1, odd_stage.alphaStage,
                                  odd_stage.rasSwap, odd_stage.texMapSwap);
      else
        writer.skip(5);

      dl.setTevIndirect(i, even_stage.indirectStage);
      if (i + 1 < stages_count)
        dl.setTevIndirect(i + 1, odd_stage.indirectStage);
      else
        writer.skip(5);

      writer.skip(3); // 3

      MAYBE_UNUSED const auto couple_len = writer.tell() - couple_start;
      DebugReport("CoupleLen: %u\n", (unsigned)couple_len);
      assert(couple_len == 48);
    }
    writer.skip(48 * (8 - stages_count_rounded / 2));

    MAYBE_UNUSED const auto end = writer.tell();
    MAYBE_UNUSED const auto shader_size = writer.tell() - shader_start;
    assert(shader_size == 0x200);
  };
  // Shaders
  {
    if (shaders.size() == 0)
      return;

    u32 dict_ofs = Dictionaries.at("Shaders");
    QDictionary _dict;
    _dict.mNodes.resize(mdl.getMaterials().size() + 1);

    for (std::size_t i = 0; i < shaders.size(); ++i) {
      writer.alignTo(32); //
      for (int j = 0; j < matToShaderMap.size(); ++j) {
        if (matToShaderMap[j] == i) {
          _dict.mNodes[j + 1].setDataDestination(writer.tell());
          _dict.mNodes[j + 1].setName(
              mdl.getMaterials()[j].IGCMaterial::getName());
        }
      }
      const auto backpatch = writePlaceholder(writer); // size
      writer.write<s32>(mdl_start - backpatch);        // mdl0 offset
      write_shader(shaders[i], backpatch, i);
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
              const auto* buf =
                  mdl.getBuf_Pos().findByName(mesh.mPositionBuffer);
              assert(buf);
              set_quant(buf->mQuantize);
              break;
            }
            case VA::Color0: {
              const auto* buf =
                  mdl.getBuf_Clr().findByName(mesh.mColorBuffer[0]);
              assert(buf);
              set_quant(buf->mQuantize);
              break;
            }
            case VA::Color1: {
              const auto* buf =
                  mdl.getBuf_Clr().findByName(mesh.mColorBuffer[1]);
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

            std::pair<librii::gx::VertexAttribute, librii::gx::VQuantization>
                tmp{attr, quant};
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
        const auto [nvtx, ntri] = computeVertTriCounts(mesh);
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

void readModel(Model& mdl, oishii::BinaryReader& reader,
               kpi::IOTransaction& transaction,
               const std::string& transaction_path) {
  const auto start = reader.tell();
  bool isValid = true;

  reader.expectMagic<'MDL0', false>();

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

  mdl.setName(readName(reader, start));

  const auto infoPos = reader.tell();
  reader.skip(8); // Ignore size, ofsMode
  mdl.mScalingRule = static_cast<librii::g3d::ScalingRule>(reader.read<u32>());
  mdl.mTexMtxMode =
      static_cast<librii::g3d::TextureMatrixMode>(reader.read<u32>());

  reader.readX<u32, 2>(); // number of vertices, number of triangles
  mdl.sourceLocation = readName(reader, infoPos);
  reader.read<u32>(); // number of view matrices

  // const auto [bMtxArray, bTexMtxArray, bBoundVolume] =
  reader.readX<u8, 3>();
  mdl.mEvpMtxMode =
      static_cast<librii::g3d::EnvelopeMatrixMode>(reader.read<u8>());

  // const s32 ofsBoneTable =
  reader.read<s32>();

  mdl.aabb.min << reader;
  mdl.aabb.max << reader;

  auto readDict = [&](u32 xofs, auto handler) {
    if (xofs) {
      reader.seekSet(start + xofs);
      Dictionary _dict(reader);
      for (std::size_t i = 1; i < _dict.mNodes.size(); ++i) {
        const auto& dnode = _dict.mNodes[i];
        assert(dnode.mDataDestination);
        reader.seekSet(dnode.mDataDestination);
        handler(dnode);
      }
    }
  };

  u32 bone_id = 0;
  readDict(secOfs.ofsBones, [&](const DictionaryNode& dnode) {
    auto& bone = mdl.getBones().add();
    reader.skip(8); // skip size and mdl offset
    bone.setName(readName(reader, dnode.mDataDestination));
    const u32 id = reader.read<u32>();
    (void)id;
    assert(id == bone_id);
    ++bone_id;
    bone.matrixId = reader.read<u32>();
    const auto bone_flag = reader.read<u32>();
    bone.billboardType = reader.read<u32>();
    reader.read<u32>(); // refId
    bone.mScaling << reader;
    bone.mRotation << reader;
    bone.mTranslation << reader;

    bone.mVolume.min << reader;
    bone.mVolume.max << reader;

    auto readHierarchyElement = [&]() {
      const auto ofs = reader.read<s32>();
      if (ofs == 0)
        return -1;
      // skip to id
      oishii::Jump<oishii::Whence::Set>(reader,
                                        dnode.mDataDestination + ofs + 12);
      return static_cast<s32>(reader.read<u32>());
    };
    bone.mParent = readHierarchyElement();
    reader.skip(12); // Skip sibling and child links -- we recompute it all
    reader.skip(2 * ((3 * 4) * sizeof(f32))); // skip matrices

    setFromFlag(bone, bone_flag);
  });
  // Compute children
  for (int i = 0; i < mdl.getBones().size(); ++i) {
    if (const auto parent_id = mdl.getBones()[i].mParent; parent_id >= 0) {
      if (parent_id >= mdl.getBones().size()) {
        printf("Invalidly large parent index..\n");
        break;
      }
      mdl.getBones()[parent_id].mChildren.push_back(i);
    }
  }
  readDict(secOfs.ofsBuffers.position, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Pos().add(), reader);
  });
  readDict(secOfs.ofsBuffers.normal, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Nrm().add(), reader);
  });
  readDict(secOfs.ofsBuffers.color, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Clr().add(), reader);
  });
  readDict(secOfs.ofsBuffers.uv, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Uv().add(), reader);
  });
  // TODO: Fur

  readDict(secOfs.ofsMaterials, [&](const DictionaryNode& dnode) {
    auto& mat = mdl.getMaterials().add();
    const auto start = reader.tell();

    reader.read<u32>(); // size
    reader.read<s32>(); // mdl offset
    mat.name = readName(reader, start);
    mat.id = reader.read<u32>();
    mat.flag = reader.read<u32>();

    // Gen info
    mat.texGens.resize(reader.read<u8>());
    auto nColorChan = reader.read<u8>();
    if (nColorChan >= 2)
      nColorChan = 2;
    mat.mStages.resize(reader.read<u8>());
    mat.indirectStages.resize(reader.read<u8>());
    mat.cullMode = static_cast<librii::gx::CullMode>(reader.read<u32>());

    // Misc
    mat.earlyZComparison = reader.read<u8>();
    mat.lightSetIndex = reader.read<s8>();
    mat.fogIndex = reader.read<s8>();
    reader.read<u8>();
    const auto indSt = reader.tell();
    for (u8 i = 0; i < mat.indirectStages.size(); ++i) {
      librii::g3d::G3dIndConfig cfg;
      cfg.method = static_cast<librii::g3d::G3dIndMethod>(reader.read<u8>());
      cfg.normalMapLightRef = reader.peekAt<s8>(4);
      mat.indConfig.push_back(cfg);
    }
    reader.seekSet(indSt + 4 + 4);

    const auto ofsTev = reader.read<s32>();
    const auto nTex = reader.read<u32>();
    assert(nTex <= 8);
    const auto [ofsSamplers, ofsFur, ofsUserData, ofsDisplayLists] =
        reader.readX<s32, 4>();
    if (ofsFur || ofsUserData)
      printf(
          "Warning: Material %s uses Fur or UserData which is unsupported!\n",
          mat.name.c_str());

    // Texture and palette objects are set on runtime.
    reader.skip((4 + 8 * 12) + (4 + 8 * 32));

    // Texture transformations
    reader.read<u32>(); // skip flag, TODO: verify
    const u32 mtxMode = reader.read<u32>();
    const std::array<libcube::GCMaterialData::CommonTransformModel, 3> cvtMtx{
        libcube::GCMaterialData::CommonTransformModel::Maya,
        libcube::GCMaterialData::CommonTransformModel::XSI,
        libcube::GCMaterialData::CommonTransformModel::Max};
    const auto xfModel = cvtMtx[mtxMode];

    for (u8 i = 0; i < nTex; ++i) {
      libcube::GCMaterialData::TexMatrix mtx;
      mtx.scale << reader;
      mtx.rotate = glm::radians(reader.read<f32>());
      mtx.translate << reader;
      mat.texMatrices.push_back(mtx);
    }
    reader.seekSet(start + 0x250);
    for (u8 i = 0; i < nTex; ++i) {
      auto* mtx = &mat.texMatrices[i];

      mtx->camIdx = reader.read<s8>();
      mtx->lightIdx = reader.read<s8>();
      // printf("[read] CamIDX: %i, LIDX:%i\n", (signed)mtx->camIdx,
      //        (signed)mtx->lightIdx);
      const u8 mapMode = reader.read<u8>();

      mtx->transformModel = xfModel;
      mtx->option = libcube::GCMaterialData::CommonMappingOption::NoSelection;
      // Projection needs to be copied from texgen

      switch (mapMode) {
      default:
      case 0:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::Standard;
        break;
      case 1:
        mtx->method =
            libcube::GCMaterialData::CommonMappingMethod::EnvironmentMapping;
        break;
      case 2:
        mtx->method =
            libcube::GCMaterialData::CommonMappingMethod::ViewProjectionMapping;
        break;
      case 3:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::
            EnvironmentLightMapping;
        break;
      case 4:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::
            EnvironmentSpecularMapping;
        break;
      // EGG
      case 5:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::
            ManualProjectionMapping;
        break;
      }

      reader.read<u8>(); // effect mtx flag
      // G3D effectmatrix is 3x4. J3D is 4x4
      reader.skip(3 * 4 * sizeof(f32));
      // for (auto& f : mtx->effectMatrix)
      //   f = reader.read<f32>();
    }
    // reader.seek((8 - nTex)* (4 + (4 * 4 * sizeof(f32))));
    reader.seekSet(start + 0x3f0);
    mat.chanData.resize(0);
    for (u8 i = 0; i < nColorChan; ++i) {
      // skip runtime flag
      const auto flag = reader.read<u32>();
      (void)flag;
      librii::gx::Color matClr, ambClr;
      matClr.r = reader.read<u8>();
      matClr.g = reader.read<u8>();
      matClr.b = reader.read<u8>();
      matClr.a = reader.read<u8>();

      ambClr.r = reader.read<u8>();
      ambClr.g = reader.read<u8>();
      ambClr.b = reader.read<u8>();
      ambClr.a = reader.read<u8>();

      mat.chanData.push_back({matClr, ambClr});
      librii::gpu::LitChannel tmp;
      // Color
      tmp.hex = reader.read<u32>();
      mat.colorChanControls.push_back(tmp);
      // Alpha
      tmp.hex = reader.read<u32>();
      mat.colorChanControls.push_back(tmp);
    }

    // TEV
    reader.seekSet(start + ofsTev + 16);
    std::array<u8, 8> coord_map_lut;
    for (auto& e : coord_map_lut)
      e = reader.read<u8>();
    // printf(">>>>> Coord->Map LUT:\n");
    // for (auto e : coord_map_lut)
    //   printf("%u ", (unsigned)e);
    // printf("\n");
    bool error = false;
    {
      u8 last = 0;
      for (auto e : coord_map_lut) {
        error |= e <= last;
        last = e;
      }
    }
    // assert(!error && "Invalid sampler configuration");
    reader.seekSet(start + ofsTev + 32);
    {

      librii::gpu::QDisplayListShaderHandler shaderHandler(mat,
                                                           mat.mStages.size());
      librii::gpu::RunDisplayList(reader, shaderHandler,
                                  shaderDlSizes[mat.mStages.size()]);
    }

    // Samplers
    reader.seekSet(start + ofsSamplers);
    assert(mat.texGens.size() == nTex);
    for (u8 i = 0; i < nTex; ++i) {
      libcube::GCMaterialData::SamplerData sampler;

      const auto sampStart = reader.tell();
      sampler.mTexture = readName(reader, sampStart);
      sampler.mPalette = readName(reader, sampStart);
      reader.skip(8);     // skip runtime pointers
      reader.read<u32>(); // skip tex id for now
      reader.read<u32>(); // skip tlut id for now
      sampler.mWrapU =
          static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
      sampler.mWrapV =
          static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
      sampler.mMinFilter =
          static_cast<librii::gx::TextureFilter>(reader.read<u32>());
      sampler.mMagFilter =
          static_cast<librii::gx::TextureFilter>(reader.read<u32>());
      sampler.mLodBias = reader.read<f32>();
      sampler.mMaxAniso =
          static_cast<librii::gx::AnisotropyLevel>(reader.read<u32>());
      sampler.bBiasClamp = reader.read<u8>();
      sampler.bEdgeLod = reader.read<u8>();
      reader.skip(2);
      mat.samplers.push_back(std::move(sampler));
    }

    // Display Lists
    reader.seekSet(start + ofsDisplayLists);
    {
      librii::gpu::QDisplayListMaterialHandler matHandler(mat);

      // Pixel data
      librii::gpu::RunDisplayList(reader, matHandler, 32);
      mat.alphaCompare = matHandler.mGpuMat.mPixel.mAlphaCompare;
      mat.zMode = matHandler.mGpuMat.mPixel.mZMode;
      mat.blendMode = matHandler.mGpuMat.mPixel.mBlendMode;
      // TODO: Dst alpha

      // Uniform data
      librii::gpu::RunDisplayList(reader, matHandler, 128);
      mat.tevColors[0] = {0xff, 0xff, 0xff, 0xff};
      for (int i = 0; i < 3; ++i) {
        mat.tevColors[i + 1] = matHandler.mGpuMat.mShaderColor.Registers[i + 1];
      }
      for (int i = 0; i < 4; ++i) {
        mat.tevKonstColors[i] = matHandler.mGpuMat.mShaderColor.Konstants[i];
      }

      // Indirect data
      librii::gpu::RunDisplayList(reader, matHandler, 64);
      for (u8 i = 0; i < mat.indirectStages.size(); ++i) {
        const auto& curScale =
            matHandler.mGpuMat.mIndirect.mIndTexScales[i > 1 ? i - 2 : i];
        mat.indirectStages[i].scale = librii::gx::IndirectTextureScalePair{
            static_cast<librii::gx::IndirectTextureScalePair::Selection>(
                curScale.ss0),
            static_cast<librii::gx::IndirectTextureScalePair::Selection>(
                curScale.ss1)};

        mat.mIndMatrices.push_back(
            matHandler.mGpuMat.mIndirect.mIndMatrices[i]);
      }

      const std::array<u32, 9> texGenDlSizes{
          0,        // 0
          32,       // 1
          64,  64,  // 2, 3
          96,  96,  // 4, 5
          128, 128, // 6, 7
          160       // 8
      };
      librii::gpu::RunDisplayList(reader, matHandler,
                                  texGenDlSizes[mat.texGens.size()]);
      for (u8 i = 0; i < mat.texGens.size(); ++i) {
        mat.texGens[i] = matHandler.mGpuMat.mTexture[i];
        mat.texMatrices[i].projection = mat.texGens[i].func;
      }
    }
  });

  readDict(secOfs.ofsMeshes, [&](const DictionaryNode& dnode) {
    auto& poly = mdl.getMeshes().add();
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
                  [&](s16 hid) { return mdl.getBuf_Pos()[hid].getName(); });
    readBufHandle(poly.mNormalBuffer,
                  [&](s16 hid) { return mdl.getBuf_Nrm()[hid].getName(); });
    for (int i = 0; i < 2; ++i) {
      readBufHandle(poly.mColorBuffer[i],
                    [&](s16 hid) { return mdl.getBuf_Clr()[hid].getName(); });
    }
    for (int i = 0; i < 8; ++i) {
      readBufHandle(poly.mTexCoordBuffer[i],
                    [&](s16 hid) { return mdl.getBuf_Uv()[hid].getName(); });
    }
    isValid &= reader.read<s32>() == -1; // fur
    reader.read<s32>();                  // matrix usage

    primitiveSetup.seekTo(reader);
    librii::gpu::QDisplayListVertexSetupHandler vcdHandler;
    librii::gpu::RunDisplayList(reader, vcdHandler, primitiveSetup.buf_size);

    for (u32 i = 0; i < (u32)librii::gx::VertexAttribute::Max; ++i) {
      if (poly.mVertexDescriptor.mBitfield & (1 << i)) {
        if (i == 0) {
          transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                               "Unsuported attribute");
          transaction.state = kpi::TransactionState::Failure;
          return;
        }
        const auto stat = vcdHandler.mGpuMesh.VCD.GetVertexArrayStatus(
            i - (u32)librii::gx::VertexAttribute::Position);
        const auto att = static_cast<librii::gx::VertexAttributeType>(stat);
        assert(att != librii::gx::VertexAttributeType::None);
        poly.mVertexDescriptor.mAttributes[(librii::gx::VertexAttribute)i] =
            att;
      }
    }
    struct QDisplayListMeshHandler final
        : public librii::gpu::QDisplayListHandler {
      void onCommandDraw(oishii::BinaryReader& reader,
                         librii::gx::PrimitiveType type, u16 nverts) override {
        if (mErr)
          return;

        if (mPoly.mMatrixPrimitives.empty())
          mPoly.mMatrixPrimitives.push_back(MatrixPrimitive{});
        auto& prim = mPoly.mMatrixPrimitives.back().mPrimitives.emplace_back(
            librii::gx::IndexedPrimitive{});
        prim.mType = type;
        prim.mVertices.resize(nverts);
        for (auto& vert : prim.mVertices) {
          for (u32 i = 0;
               i < static_cast<u32>(librii::gx::VertexAttribute::Max); ++i) {
            if (mPoly.mVertexDescriptor.mBitfield & (1 << i)) {
              const auto attr = static_cast<librii::gx::VertexAttribute>(i);
              switch (mPoly.mVertexDescriptor.mAttributes[attr]) {
              case librii::gx::VertexAttributeType::Direct:
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
      QDisplayListMeshHandler(Polygon& poly) : mPoly(poly) {}
      bool mErr = false;
      Polygon& mPoly;
    } meshHandler(poly);
    primitiveData.seekTo(reader);
    librii::gpu::RunDisplayList(reader, meshHandler, primitiveData.buf_size);
    if (meshHandler.mErr) {
      transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                           "Mesh unsupported.");
      transaction.state = kpi::TransactionState::Failure;
    }
  });

  if (transaction.state == kpi::TransactionState::Failure)
    return;

  readDict(secOfs.ofsRenderTree, [&](const DictionaryNode& dnode) {
    enum class Tree { DrawOpa, DrawXlu, Node, Other } tree = Tree::Other;
    std::array<std::pair<std::string_view, Tree>, 3> type_lut{
        std::pair<std::string_view, Tree>{"DrawOpa", Tree::DrawOpa},
        std::pair<std::string_view, Tree>{"DrawXlu", Tree::DrawXlu},
        std::pair<std::string_view, Tree>{"NodeTree", Tree::Node}};

    if (const auto it = std::find_if(
            type_lut.begin(), type_lut.end(),
            [&](const auto& tuple) { return tuple.first == dnode.mName; });
        it != type_lut.end()) {
      tree = it->second;
    }
    int base_matrix_id = -1;

    // printf("digraph {\n");
    while (reader.tell() < reader.endpos()) {
      const auto cmd = static_cast<RenderCommand>(reader.read<u8>());
      switch (cmd) {
      case RenderCommand::NoOp:
        break;
      case RenderCommand::Return:
        return;
      case RenderCommand::Draw: {
        Bone::Display disp;
        disp.matId = reader.readUnaligned<u16>();
        disp.polyId = reader.readUnaligned<u16>();
        const auto boneIdx = reader.readUnaligned<u16>();
        disp.prio = reader.readUnaligned<u8>();
        mdl.getBones()[boneIdx].addDisplay(disp);

        // While with this setup, materials could be XLU and OPA, in
        // practice, they're not.
        auto& mat = mdl.getMaterials()[disp.matId];
        const bool xlu_mat = mat.flag & 0x80000000;
        auto& poly = mdl.getMeshes()[disp.polyId];

        if ((tree == Tree::DrawOpa && xlu_mat) ||
            (tree == Tree::DrawXlu && !xlu_mat)) {
          char warn_msg[1024]{};
          const auto mat_name = ((riistudio::lib3d::Material&)mat).getName();
          snprintf(warn_msg, sizeof(warn_msg),
                   "Material %u \"%s\" is rendered in the %s pass (with mesh "
                   "%u \"%s\"), but is marked as %s.",
                   disp.matId, mat_name.c_str(),
                   xlu_mat ? "Opaue" : "Translucent", disp.polyId,
                   poly.getName().c_str(), !xlu_mat ? "Opaque" : "Translucent");
          reader.warnAt(warn_msg, reader.tell() - 8, reader.tell());
          transaction.callback(kpi::IOMessageClass::Warning,
                               transaction_path + "materials/" + mat_name,
                               warn_msg);
        }
        mat.setXluPass(tree == Tree::DrawXlu);
      } break;
      case RenderCommand::NodeDescendence: {
        const auto boneIdx = reader.readUnaligned<u16>();
        // const auto parentMtxIdx =
        reader.readUnaligned<u16>();

        const auto& bone = mdl.getBones()[boneIdx];
        const auto matrixId = bone.matrixId;

        // Hack: Base matrix is first copied to end, first instruction
        if (base_matrix_id == -1) {
          base_matrix_id = matrixId;
        } else {
          auto& drws = mdl.mDrawMatrices;
          if (drws.size() <= matrixId) {
            drws.resize(matrixId + 1);
          }
          // TODO: Start at 100?
          drws[matrixId].mWeights.emplace_back(boneIdx, 1.0f);
        }
        // printf("BoneIdx: %u (MatrixIdx: %u), ParentMtxIdx: %u\n",
        // (u32)boneIdx,
        //        (u32)matrixId, (u32)parentMtxIdx);
        // printf("\"Matrix %u\" -> \"Matrix %u\" [label=\"parent\"];\n",
        //        (u32)matrixId, (u32)parentMtxIdx);
      } break;
      default:
        // TODO
        break;
      }
    }
    // printf("}\n");
  });

  if (!isValid && mdl.getBones().size() > 1) {
    transaction.callback(
        kpi::IOMessageClass::Error, transaction_path,
        "BRRES file was created with BrawlBox and is invalid. It is "
        "recommended you create BRRES files here by dropping a DAE/FBX file.");
    //
    transaction.state = kpi::TransactionState::Failure;
  } else if (!isValid) {
    transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                         "Note: BRRES file was saved with BrawlBox. Certain "
                         "materials may flicker during ghost replays.");
  } else if (mdl.getBones().size() > 1) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "Rigging support is not fully tested."
                         "Rejecting file to avoid potential corruption.");
    transaction.state = kpi::TransactionState::Failure;
  }
}

} // namespace riistudio::g3d
