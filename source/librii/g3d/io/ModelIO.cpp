#include "ModelIO.hpp"

#include "CommonIO.hpp"
// XXX: Must not include DictIO.hpp when using DictWriteIO.hpp
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/ModelIO.hpp>
#include <librii/g3d/io/PolygonIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>

///// Headers of glm_io.hpp
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vendor/glm/vec2.hpp>
#include <vendor/glm/vec3.hpp>

#include <librii/math/mtx.hpp>
#include <librii/math/srt3.hpp>
#include <librii/trig/WiiTrig.hpp>

namespace librii::g3d {

Model::Model() __attribute__((weak)) = default;
Model::Model(const Model&) __attribute__((weak)) = default;
Model::Model(Model&&) noexcept __attribute__((weak)) = default;
Model::~Model() __attribute__((weak)) = default;

template <bool Named, bool bMaterial, typename T, typename U>
Result<void> writeDictionary(const std::string& name, T&& src_range, U handler,
                             RelocWriter& linker, oishii::Writer& writer,
                             u32 mdl_start, NameTable& names, int* d_cursor,
                             bool raw = false, u32 align = 4,
                             bool BetterMethod = false) {
  if (src_range.size() == 0)
    return {};

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
      TRY(handler(src_range[i], backpatch));
      writeOffsetBackpatch(writer, backpatch, backpatch);
    } else {
      TRY(handler(src_range[i], writer.tell()));
    }
  }
  {
    oishii::Jump<oishii::Whence::Set, oishii::Writer> d(writer, dict_ofs);
    linker.label(name);
    WriteDictionary(_dict, writer, names);
  }

  return {};
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
  switch (out.mQuantize.mType.generic) {
  case librii::gx::VertexBufferType::Generic::s8: {
    if (kind == librii::gx::VertexBufferKind::normal) {
      out.mQuantize.divisor = 6;
    }
    break;
  }
  case librii::gx::VertexBufferType::Generic::s16:
    if (kind == librii::gx::VertexBufferKind::normal) {
      out.mQuantize.divisor = 14;
    }
    break;
  case librii::gx::VertexBufferType::Generic::u8:
  case librii::gx::VertexBufferType::Generic::u16:
    break;
  case librii::gx::VertexBufferType::Generic::f32:
    out.mQuantize.divisor = 0;
    break;
  }
  out.mEntries.resize(TRY(reader.U16()));
  if constexpr (HasMinimum) {
    T minEnt, maxEnt;
    TRY(minEnt << reader.getUnsafe());
    TRY(maxEnt << reader.getUnsafe());

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

Result<void> BinaryModel::read(oishii::BinaryReader& unsafeReader,
                               kpi::LightIOTransaction& transaction,
                               const std::string& transaction_path,
                               bool& isValid) {
  rsl::SafeReader reader(unsafeReader);
  const auto start = reader.tell();
  TRY(reader.Magic("MDL0"));
  [[maybe_unused]] const u32 fileSize = TRY(reader.U32());
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

  TRY(readDict(secOfs.ofsBones,
               [&](const librii::g3d::BetterNode& dnode) -> Result<void> {
                 auto& bone = bones.emplace_back();
                 TRY(bone.read(unsafeReader));
                 // TODO: We shouldn't need this hack
                 if (bone.matrixId < info.numViewMtx) {
                   bone.forceDisplayMatrix = true;
                 }
                 return {};
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

  // TODO: Fur

  TRY(readDict(secOfs.ofsMaterials, [&](const librii::g3d::BetterNode& dnode) {
    auto& mat = materials.emplace_back();
    return mat.read(unsafeReader, nullptr);
  }));
  u32 i = 0;
  u32 num_furpolys = 0;
  TRY(readDict(secOfs.ofsMeshes,
               [&](const librii::g3d::BetterNode& dnode) -> Result<void> {
                 auto& poly = meshes.emplace_back();
                 bool badfur = false;
                 TRY(ReadMesh(poly, reader, isValid, positions, normals, colors,
                              texcoords, transaction, transaction_path, i++,
                              &badfur));
                 if (badfur) {
                   ++num_furpolys;
                 }
                 return {};
               }));

  if (num_furpolys != 0) {
    transaction.callback(
        kpi::IOMessageClass::Warning, transaction_path,
        std::format("{} polygons are specified as having fur data, which "
                    "is unsupported and almost never used. "
                    "Likely erroneously set by BrawlBox.",
                    num_furpolys));
  }

  TRY(readDict(secOfs.ofsRenderTree,
               [&](const librii::g3d::BetterNode& dnode) -> Result<void> {
                 auto commands = TRY(ByteCodeLists::ParseStream(unsafeReader));
                 ByteCodeMethod c{dnode.name, commands};
                 bytecodes.emplace_back(c);
                 return {};
               }));

  for (auto& bone : bones) {
    bone.omitFromNodeMix = true;
  }
  for (auto& bc : bytecodes) {
    if (bc.name != "NodeMix") {
      continue;
    }
    for (auto& cmd : bc.commands) {
      if (auto* x = std::get_if<ByteCodeLists::EnvelopeMatrix>(&cmd)) {
        EXPECT(x->nodeId < bones.size());
        bones[x->nodeId].omitFromNodeMix = false;
      }
    }
    break;
  }

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
[[nodiscard]] Result<void> writeGenericBuffer(
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
  EXPECT(nComponents.has_value());

  for (auto& entry : buf.mEntries) {
    librii::gx::writeComponents(writer, entry, buf.mQuantize.mType,
                                *nComponents, buf.mQuantize.divisor);
  }
  writer.alignTo(32);
  return {};
}

void WriteShader(RelocWriter& linker, oishii::Writer& writer,
                 const BinaryTev& tev, std::size_t shader_start,
                 int shader_id) {
  rsl::info("Shader at {:x}", shader_start);
  linker.label("Shader" + std::to_string(shader_id), shader_start);

  tev.writeBody(writer);
}

Result<void> writeModel(librii::g3d::BinaryModel& bin, oishii::Writer& writer,
                        NameTable& names, std::size_t brres_start) {
  // index: polygon
  std::map<u32, std::bitset<8>> mesh_texmtx;
  bool any_texmtx = false;
  for (auto& bc : bin.bytecodes) {
    if (bc.name != "DrawOpa" && bc.name != "DrawXlu")
      continue;
    for (auto& draw : bc.commands) {
      if (auto* x = std::get_if<ByteCodeLists::Draw>(&draw)) {
        u32 matId = x->matId;
        EXPECT(matId < bin.materials.size());
        auto& mat = bin.materials[matId];
        u32 polyId = x->polyId;
        EXPECT(polyId < bin.meshes.size());
        auto& poly = bin.meshes[polyId];
        if (!poly.mVertexDescriptor
                 [gx::VertexAttribute::PositionNormalMatrixIndex]) {
          // We only care for the rigged case
          continue;
        }
        for (size_t i = 0; i < mat.dl.texGens.size(); ++i) {
          auto& tg = mat.dl.texGens[i];
          bool rigged_tg_need_texmtx = NeedTexMtx(tg);
          mesh_texmtx[polyId][i] =
              mesh_texmtx[polyId][i] || rigged_tg_need_texmtx;
          any_texmtx |= rigged_tg_need_texmtx;
        }
      }
    }
  }
  // TODO: This is a hack
  bin.info.texMtxArray = any_texmtx;

  const auto mdl_start = writer.tell();
  int d_cursor = 0;

  RelocWriter linker(writer);

  std::map<std::string, u32> Dictionaries;
  const auto write_dict = [&](const std::string& name, auto src_range,
                              auto handler, bool raw = false,
                              u32 align = 4) -> Result<void> {
    if (src_range.end() == src_range.begin())
      return {};
    int d_pos = Dictionaries.at(name);
    return writeDictionary<true, false>(name, src_range, handler, linker,
                                        writer, mdl_start, names, &d_pos, raw,
                                        align);
  };
  const auto write_dict_mat = [&](const std::string& name, auto& src_range,
                                  auto handler, bool raw = false,
                                  u32 align = 4) -> Result<void> {
    if (src_range.end() == src_range.begin())
      return {};
    int d_pos = Dictionaries.at(name);
    return writeDictionary<true, true>(name, src_range, handler, linker, writer,
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
    rsl::trace("{}: {} entries", str, dict.size());
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
    rsl::trace("{}: {}", key, val);
  }
  writer.skip(dicts_size);

  int sm_i = 0;
  TRY(write_dict(
      "TexSamplerMap", tex_sampler_mappings,
      [&](librii::g3d::TextureSamplerMapping& map,
          std::size_t start) -> Result<void> {
        writer.write<u32>(map.entries.size());
        for (int i = 0; i < map.entries.size(); ++i) {
          tex_sampler_mappings.entries[sm_i].entries[i] = writer.tell();
          // These are placeholders
          writer.write<s32>(0, /* checkmatch */ false);
          writer.write<s32>(0, /* checkmatch */ false);
        }
        ++sm_i;
        return {};
      },
      true, 4));

  TRY(write_dict(
      "RenderTree", bin.bytecodes,
      [&](const librii::g3d::ByteCodeMethod& list,
          std::size_t) -> Result<void> {
        librii::g3d::ByteCodeLists::WriteStream(writer, list.commands);
        return {};
      },
      true, 1));

  TRY(write_dict_mat("Bones", bin.bones,
                     [&](const librii::g3d::BinaryBoneData& bone,
                         std::size_t bone_start) -> Result<void> {
                       rsl::trace("Bone at {}", bone_start);
                       writer.seekSet(bone_start);
                       bone.write(names, writer, mdl_start);
                       return {};
                     }));

  TRY(write_dict_mat(
      "Materials", bin.materials,
      [&](const librii::g3d::BinaryMaterial& mat,
          std::size_t mat_start) -> Result<void> {
        linker.label("Mat" + std::to_string(mat_start), mat_start);
        TRY(mat.writeBody(writer, mat_start, names, linker,
                          tex_sampler_mappings));
        return {};
      },
      false, 4));

  // Shaders
  {
    if (bin.tevs.size() == 0)
      return {};

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

  u32 i = 0;
  TRY(write_dict(
      "Meshes", bin.meshes,
      [&](const librii::g3d::PolygonData& mesh,
          std::size_t mesh_start) -> Result<void> {
        std::bitset<8> texmtx_needed = mesh_texmtx[i];
        return WriteMesh(writer, mesh, bin, mesh_start, names, i++,
                         texmtx_needed);
      },
      false, 32));

  TRY(write_dict(
      "Buffer_Position", bin.positions,
      [&](const librii::g3d::PositionBuffer& buf, std::size_t buf_start) {
        return writeGenericBuffer(buf, writer, buf_start, names,
                                  buf.mQuantize.mComp.position,
                                  buf.mQuantize.mType.generic);
      },
      false, 32));
  TRY(write_dict(
      "Buffer_Normal", bin.normals,
      [&](const librii::g3d::NormalBuffer& buf, std::size_t buf_start) {
        return writeGenericBuffer(buf, writer, buf_start, names,
                                  buf.mQuantize.mComp.normal,
                                  buf.mQuantize.mType.generic);
      },
      false, 32));
  TRY(write_dict(
      "Buffer_Color", bin.colors,
      [&](const librii::g3d::ColorBuffer& buf, std::size_t buf_start) {
        return writeGenericBuffer(buf, writer, buf_start, names,
                                  buf.mQuantize.mComp.color,
                                  buf.mQuantize.mType.color);
      },
      false, 32));
  TRY(write_dict(
      "Buffer_UV", bin.texcoords,
      [&](const librii::g3d::TextureCoordinateBuffer& buf,
          std::size_t buf_start) {
        return writeGenericBuffer(buf, writer, buf_start, names,
                                  buf.mQuantize.mComp.texcoord,
                                  buf.mQuantize.mType.generic);
      },
      false, 32));

  linker.writeChildren();
  linker.label("MDL0_END");
  linker.resolve();

  return {};
}

} // namespace

Result<void> BinaryModel::write(oishii::Writer& writer, NameTable& names,
                                std::size_t brres_start) {
  return writeModel(*this, writer, names, brres_start);
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

librii::g3d::BoneData fromBinaryBone(const librii::g3d::BinaryBoneData& bin,
                                     auto&& bones, kpi::IOContext& ctx_,
                                     librii::g3d::ScalingRule scalingRule) {
  auto ctx = ctx_.sublet("Bone " + bin.name);
  librii::g3d::BoneData bone;
  bone.mName = bin.name;
  rsl::trace("Bone: {}", bone.mName);
  // TODO: Verify matrixId
  bone.billboardType = bin.billboardType;
  // TODO: refId
  bone.mScaling = bin.scale;
  bone.mRotation = bin.rotate;
  bone.mTranslation = bin.translate;

  bone.mVolume = bin.aabb;

  bone.mParent = bin.parent_id;
  // Skip sibling and child links -- we recompute it all

  bone.forceDisplayMatrix = bin.forceDisplayMatrix;
  bone.omitFromNodeMix = bin.omitFromNodeMix;

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

Result<librii::g3d::BinaryBoneData>
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
    EXPECT(it != siblings.end());
    // Not a cyclic linked list
    bin.sibling_left_id = it == siblings.begin() ? -1 : *(it - 1);
    bin.sibling_right_id = it == siblings.end() - 1 ? -1 : *(it + 1);
  }

  auto modelMtx = calcSrtMtx(bone, bones, scalingRule);
  auto modelMtx34 = glm::mat4x3(modelMtx);

  bin.modelMtx = modelMtx34;
  bin.inverseModelMtx =
      librii::g3d::MTXInverse(modelMtx34).value_or(glm::mat4x3(0.0f));

  bin.forceDisplayMatrix = bone.forceDisplayMatrix;
  bin.omitFromNodeMix = bone.omitFromNodeMix;

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

  Result<void> onDraw(const B::Draw& draw) {
    BoneData::DisplayCommand disp{
        .mMaterial = static_cast<u32>(draw.matId),
        .mPoly = static_cast<u32>(draw.polyId),
        .mPrio = draw.prio,
    };
    auto boneIdx = draw.boneId;
    if (boneIdx > mdl.bones.size()) {
      ctx.error("Invalid bone index in render command");
      boneIdx = 0;
      return std::unexpected("Invalid bone index in render command");
    }

    if (disp.mMaterial > mdl.meshes.size()) {
      ctx.error("Invalid material index in render command");
      disp.mMaterial = 0;
      return std::unexpected("Invalid material index in render command");
    }

    if (disp.mPoly > mdl.meshes.size()) {
      ctx.error("Invalid mesh index in render command");
      disp.mPoly = 0;
      return std::unexpected("Invalid mesh index in render command");
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
      const bool xlu_mat = mat.xlu;
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
    return {};
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

  bool isError() const { return error; }

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
  bool error = false;
};

inline std::set<s16> gcomputeShapeMtxRef(auto&& meshes) {
  std::set<s16> shapeRefMtx;
  for (auto&& mesh : meshes) {
    // TODO: Do we need to check currentMatrixEmbedded flag?
    if (mesh.mCurrentMatrix != -1) {
      shapeRefMtx.insert(mesh.mCurrentMatrix);
      continue;
    }
    // TODO: Presumably mCurrentMatrix (envelope mode) precludes blended weight
    // mode?
    for (auto& mp : mesh.mMatrixPrimitives) {
      for (auto& w : mp.mDrawMatrixIndices) {
        if (w == -1) {
          continue;
        }
        shapeRefMtx.insert(w);
      }
    }
  }
  return shapeRefMtx;
}
inline std::set<s16> gcomputeDisplayMatricesSubset(
    const auto& meshes, const auto& bones,
    auto getMatrixId = [](auto& x) { return x.matrixId; }) {
  std::set<s16> displayMatrices = gcomputeShapeMtxRef(meshes);
  for (int i = 0; i < bones.size(); ++i) {
    const auto& bone = bones[i];
    if (!displayMatrices.contains(getMatrixId(bone, i))) {
      // Assume non-view matrices are appended to end
      // HACK: Permit leaves to match map_model.brres
      if (!bone.forceDisplayMatrix) {
        continue;
      }
      displayMatrices.insert(getMatrixId(bone, i));
    }
  }
  return displayMatrices;
}

Result<void> processModel(const BinaryModel& binary_model,
                          kpi::LightIOTransaction& transaction,
                          std::string_view transaction_path, Model& mdl) {
  using namespace librii::g3d;
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
      auto displayMatrices = gcomputeDisplayMatricesSubset(
          binary_model.meshes, binary_model.bones,
          [](auto& x, int i) { return x.matrixId; });
      ctx.request(info.numViewMtx == displayMatrices.size(),
                  "Model header specifies {} display matrices, but the mesh "
                  "data only references {} display matrices.",
                  info.numViewMtx, displayMatrices.size());
      auto ref = gcomputeShapeMtxRef(binary_model.meshes);
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
                      [](auto& m) { return m.needsAnyTextureMtx(); });
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
    auto& bone = binary_model.bones[i];
    // CTools seemingly doesn't do this???
    if (bone.id != i) {
      ctx.error("Bone IDs are desynced. Is this a CTools minimap???");
    }
    auto new_bone = fromBinaryBone(bone, binary_model.bones, ctx,
                                   binary_model.info.scalingRule);
    mdl.bones.push_back(new_bone);
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
        TRY(helper.onDraw(*draw));
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

[[nodiscard]] Result<void>
BuildRenderLists(const Model& mdl,
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
    EXPECT(parent < std::ssize(mdl.bones));
    librii::g3d::ByteCodeLists::NodeDescendence desc{
        .boneId = static_cast<u16>(i),
        .parentMtxId = static_cast<u16>(parent >= 0 ? boneToMatrix[parent] : 0),
    };
    nodeTree.commands.push_back(desc);
  }

  auto write_drw = [&](const DrawMatrix& drw, size_t i) -> Result<void> {
    if (drw.mWeights.size() > 1) {
      librii::g3d::ByteCodeLists::NodeMix mix{
          .mtxId = static_cast<u16>(i),
      };
      for (auto& weight : drw.mWeights) {
        EXPECT(weight.boneId < mdl.bones.size());
        mix.blendMatrices.push_back(
            librii::g3d::ByteCodeLists::NodeMix::BlendMtx{
                .mtxId = static_cast<u16>(boneToMatrix[weight.boneId]),
                .ratio = weight.weight,
            });
      }
      nodeMix.commands.push_back(mix);
    } else {
      EXPECT(drw.mWeights[0].boneId < mdl.bones.size());
      librii::g3d::ByteCodeLists::EnvelopeMatrix evp{
          .mtxId = static_cast<u16>(i),
          .nodeId = static_cast<u16>(drw.mWeights[0].boneId),
      };
      nodeMix.commands.push_back(evp);
    }
    return {};
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
      // Official models seem to omit unecessary NODEMIXES
      if (mdl.bones[i].omitFromNodeMix) {
        continue;
      }
      auto& drw = drawMatrices[boneToMatrix[i]];
      TRY(write_drw(drw, boneToMatrix[i]));
    }
    for (size_t i = 0; i < mdl.matrices.size(); ++i) {
      auto& drw = drawMatrices[i];
      if (drw.mWeights.size() == 1) {
        // Written in pre-pass
        continue;
      }
      TRY(write_drw(drw, i));
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

  return {};
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

Result<librii::g3d::BinaryModel> toBinaryModel(const Model& mdl) {
  std::set<s16> shapeRefMtx = computeShapeMtxRef(mdl.meshes);
  rsl::debug("shapeRefMtx: {}, Before: {}", shapeRefMtx.size(),
             mdl.matrices.size());
  if (!mdl.meshes.empty()) {
    // EXPECT(shapeRefMtx.size() == mdl.matrices.size());
  }
  std::vector<DrawMatrix> drawMatrices = mdl.matrices;
  std::vector<u32> boneToMatrix;
  rsl::debug("# Bones = {}", mdl.bones.size());
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
      rsl::debug("Adding drawmtx {} for bone {} since no singlebound found",
                 drawMatrices.size(), i);
      drawMatrices.push_back(mtx);
    } else {
      boneToMatrix.push_back(it);
    }
  }
  auto getMatrixId = [&](auto& x, int i) { return boneToMatrix[i]; };
  std::set<s16> displayMatrices =
      gcomputeDisplayMatricesSubset(mdl.meshes, mdl.bones, getMatrixId);
  rsl::debug("boneToMatrix: {}, displayMatrices: {}, drawMatrices: {}",
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
  librii::g3d::BinaryModel bin{
      .name = mdl.name,
      .positions = mdl.positions,
      .normals = mdl.normals,
      .colors = mdl.colors,
      .texcoords = mdl.texcoords,
      .tevs = tevs,
      .meshes = mdl.meshes,
  };
  for (auto [index, value] : rsl::enumerate(mdl.materials)) {
    bin.materials.push_back(librii::g3d::toBinMat(value, index));
  }
  for (auto&& [index, value] : rsl::enumerate(mdl.bones)) {
    auto bb = toBinaryBone(value, bones, index, mdl.info.scalingRule,
                           boneToMatrix[index]);
    bin.bones.emplace_back(TRY(bb));
  }

  for (size_t i = 0; i < shader_allocator.matToShaderMap.size(); ++i) {
    bin.materials[i].tevId = shader_allocator.matToShaderMap[i];
  }

  // Compute ModelInfo
  {
    const auto [nVert, nTri] = librii::gx::computeVertTriCounts(mdl.meshes);
    bool nrmMtx = std::ranges::any_of(
        mdl.meshes, [](auto& m) { return m.needsNormalMtx(); });
    bool texMtx = std::ranges::any_of(
        mdl.meshes, [](auto& m) { return m.needsAnyTextureMtx(); });
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
        EXPECT(bone.matrixId < lut.size());
        lut[bone.matrixId] = i;
      }
    }
    bin.info = info;
  }
  TRY(BuildRenderLists(mdl, bin.bytecodes, drawMatrices, boneToMatrix));

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
