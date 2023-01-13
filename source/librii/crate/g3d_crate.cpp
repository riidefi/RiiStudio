#include "g3d_crate.hpp"
#include <core/util/timestamp.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>

IMPORT_STD;

template <>
struct std::formatter<std::filesystem::path> : std::formatter<std::string> {
  auto format(std::filesystem::path p, format_context& ctx) {
    return formatter<std::string>::format(std::format("{}", p.string()), ctx);
  }
};

namespace librii::crate {

Result<g3d::G3dMaterialData> ReadMDL0Mat(std::span<const u8> file) {
  std::vector<u8> bruh;
  oishii::DataProvider provider(std::move(bruh));
  oishii::ByteView view(file, provider, "MDL0Mat");
  oishii::BinaryReader reader(std::move(view));

  g3d::G3dMaterialData mat;
  const bool ok = g3d::readMaterial(mat, reader, /* ignore_tev */ true);
  if (!ok) {
    return std::unexpected("Failed to read MDL0Mat file");
  }

  return mat;
}

Result<std::vector<u8>> WriteMDL0Mat(const g3d::G3dMaterialData& mat) {
  oishii::Writer writer(0);

  g3d::NameTable names;
  g3d::RelocWriter linker(writer);
  g3d::TextureSamplerMappingManager tex_sampler_mappings;
  // This doesn't match the final output order; that is by the order in the
  // textures folder. But it means we don't need to depend on a textures folder
  // to determine this order.
  for (int s = 0; s < mat.samplers.size(); ++s) {
    if (tex_sampler_mappings.contains(mat.samplers[s].mTexture))
      continue;
    tex_sampler_mappings.add_entry(mat.samplers[s].mTexture, mat.name, s);
  }
#if 0
  for (int sm_i = 0; sm_i < tex_sampler_mappings.size(); ++sm_i) {
    auto& map = tex_sampler_mappings[sm_i];
    for (int i = 0; i < map.entries.size(); ++i) {
      tex_sampler_mappings.entries[sm_i].entries[i] = writer.tell();
      writer.write<s32>(0);
      writer.write<s32>(0);
    }
  }
#endif
  // NOTE: tex_sampler_mappings will point to garbage
  linker.label("here");
  linker.writeReloc<s32>("here", "end"); // size
  writer.write<s32>(0);                  // mdl offset
  // Differences:
  // - Crate outputs the BRRES offset and mat index here
  TRY(g3d::WriteMaterialBody(0, writer, names, mat, 0, linker,
                             tex_sampler_mappings));
  const auto end = writer.tell();
  {
    names.poolNames();
    names.resolve(end);
    writer.seekSet(end);
    for (auto p : names.mPool)
      writer.write<u8>(p);
  }
  linker.label("end");
  writer.alignTo(64);
  linker.resolve();
  linker.printLabels();
  return writer.takeBuf();
}

Result<g3d::G3dShader> ReadMDL0Shade(std::span<const u8> file) {
  // Skip first 8 bytes
  // TODO: Clean this up
  file = file.subspan(8);

  std::vector<u8> bruh;
  oishii::DataProvider provider(std::move(bruh));
  oishii::ByteView view(file, provider, "MDLShade");
  oishii::BinaryReader reader(std::move(view));

  // This function is a bit more complex, as we normally assume materials are
  // read before shaders; here we don't make such assumptions.

  gx::LowLevelGxMaterial gx_mat;
  // So the DL parser doesn't discard IndOrders
  gx_mat.indirectStages.resize(4);
  // Trust the stage count since we have not necessarily read the material yet.
  g3d::ReadTev(gx_mat, reader, /* tev_addr */ 0,
               /* trust_stagecount */ true, /* brawlbox_bug */ true);

  g3d::G3dShader shader;

  shader.mSwapTable = gx_mat.mSwapTable;

  shader.mIndirectOrders.resize(gx_mat.indirectStages.size());
  for (size_t i = 0; i < gx_mat.indirectStages.size(); ++i) {
    shader.mIndirectOrders[i] = gx_mat.indirectStages[i].order;
  }
  // The true size is set by the material later

  shader.mStages = gx_mat.mStages;

  return shader;
}

std::vector<u8> WriteMDL0Shade(const g3d::G3dMaterialData& mat) {
  oishii::Writer writer(0);

  g3d::NameTable names;
  g3d::RelocWriter linker(writer);

  librii::g3d::G3dShader shader(mat);

  linker.label("here");
  linker.writeReloc<s32>("here", "end"); // size
  writer.write<s32>(0);                  // mdl offset
  // Differences:
  // - Crate outputs the BRRES offset and index here
  g3d::WriteTevBody(writer, 0, shader);
  const auto end = writer.tell();
  {
    names.poolNames();
    names.resolve(end);
    writer.seekSet(end);
    for (auto p : names.mPool)
      writer.write<u8>(p);
  }
  linker.label("end");
  writer.alignTo(4);
  linker.resolve();
  linker.printLabels();
  return writer.takeBuf();
}

Result<g3d::TextureData, std::string> ReadTEX0(std::span<const u8> file) {
  g3d::TextureData tex;
  // TODO: We trust the .tex0-file provided name to be correct
  const bool ok = g3d::ReadTexture(tex, file, "");
  if (!ok) {
    return std::unexpected(
        "Failed to parse TEX0: g3d::ReadTexture returned false");
  }
  return tex;
}

class SimpleRelocApplier {
public:
  SimpleRelocApplier(std::vector<u8>& buf) : mBuf(buf) {}
  ~SimpleRelocApplier() = default;

  void apply(g3d::NameReloc reloc, u32 structure_offset) {
    // Transform from structure-space to buffer-space
    reloc = g3d::RebasedNameReloc(reloc, structure_offset);
    // Write to end of mBuffer
    const u32 string_location = writeName(reloc.name);
    mBuf.resize(roundUp(mBuf.size(), 4));
    // Resolve pointer
    writePointer(reloc, string_location);
  }

private:
  void writePointer(g3d::NameReloc& reloc, u32 string_location) {
    const u32 pointer_location = reloc.offset_of_pointer_in_struct;
    const u32 structure_location = reloc.offset_of_delta_reference;
    rsl::store<s32>(string_location - structure_location, mBuf,
                    pointer_location);
  }
  u32 writeName(std::string_view name) {
    const u32 sz = name.size();
    mBuf.push_back((sz & 0xff000000) >> 24);
    mBuf.push_back((sz & 0x00ff0000) >> 16);
    mBuf.push_back((sz & 0x0000ff00) >> 8);
    mBuf.push_back((sz & 0x000000ff) >> 0);
    const u32 string_location = mBuf.size();
    mBuf.insert(mBuf.end(), name.begin(), name.end());
    mBuf.push_back(0);
    return string_location;
  }
  std::vector<u8>& mBuf;
};

std::vector<u8> WriteTEX0(const g3d::TextureData& tex) {
  auto memory_request = g3d::CalcTextureBlockData(tex);
  std::vector<u8> buffer(memory_request.size);

  g3d::NameReloc reloc;
  g3d::WriteTexture(buffer, tex, 0, reloc);
  SimpleRelocApplier applier(buffer);
  applier.apply(reloc, 0 /* structure offset */);

  return buffer;
}

Result<g3d::BinarySrt, std::string> ReadSRT0(std::span<const u8> file) {
  g3d::SrtAnimationArchive arc;
  oishii::DataProvider cringe(file | rsl::ToList());
  oishii::BinaryReader reader(cringe.slice());
  auto ok = arc.read(reader);
  if (!ok) {
    return std::unexpected("Failed to parse SRT0: g3d::ReadSrtFile returned " +
                           ok.error());
  }
  return arc;
}

Result<std::vector<u8>> WriteSRT0(const g3d::SrtAnimationArchive& arc) {
  oishii::Writer writer(0);

  g3d::NameTable names;
  // g3d::RelocWriter linker(writer);

  TRY(arc.write(writer, names, 0));
  const auto end = writer.tell();
  {
    names.poolNames();
    names.resolve(end);
    writer.seekSet(end);
    for (auto p : names.mPool)
      writer.write<u8>(p);
  }
  writer.alignTo(4);
  // linker.resolve();
  // linker.printLabels();
  return writer.takeBuf();
}

Result<g3d::G3dMaterialData>
ApplyG3dShaderToMaterial(const g3d::G3dMaterialData& mat,
                         const g3d::G3dShader& tev) {
  const size_t num_istage = mat.indirectStages.size();
  EXPECT(tev.mIndirectOrders.size() >= num_istage,
         "Material and shader were built for a different number of indirect "
         "stages.");

  g3d::G3dMaterialData tmp = mat;
  tmp.mSwapTable = tev.mSwapTable;
  for (size_t i = 0; i < mat.indirectStages.size(); ++i) {
    tmp.indirectStages[i].order =
        i < tev.mIndirectOrders.size() ? tev.mIndirectOrders[i] : gx::NullOrder;
  }
  // Override with material stage count
  const size_t num_tevstages = mat.mStages.size();
  tmp.mStages = tev.mStages;
  tmp.mStages.resize(num_tevstages);

  return tmp;
}

static std::string FormatRange(auto&& range, auto&& functor) {
  std::string tmp;
  for (const auto& it : range) {
    tmp += ", " + static_cast<std::string>(functor(it));
  }
  // Cut off initial ", "
  if (tmp.size() >= 2) {
    tmp = tmp.substr(2);
  }
  return tmp;
}

Result<CrateAnimationPaths>
ScanCrateAnimationFolder(std::filesystem::path path) {
  EXPECT(std::filesystem::is_directory(path),
         "ScanCrateAnimationFolder takes in a folder, not a file.");
  CrateAnimationPaths tmp;
  tmp.preset_name = path.stem().string();
  for (const auto& entry : std::filesystem::directory_iterator{path}) {
    const auto extension = entry.path().extension();
    if (extension == ".mdl0mat") {
      if (!tmp.mdl0mat.empty()) {
        return std::unexpected("Rejecting: Multiple .mdl0mat files in folder");
      }
      tmp.mdl0mat = entry.path();
    } else if (extension == ".mdl0shade") {
      if (!tmp.mdl0shade.empty()) {
        return std::unexpected(
            "Rejecting: Multiple .mdl0shade files in folder");
      }
      tmp.mdl0shade = entry.path();
    } else if (extension == ".srt0") {
      tmp.srt0.emplace_back(entry.path());
    } else if (extension == ".tex0") {
      tmp.tex0.emplace_back(entry.path());
    }
    // Ignore other extensions
  }
  if (tmp.mdl0mat.empty()) {
    return std::unexpected("Missing .mdl0mat definition");
  }
  if (tmp.mdl0shade.empty()) {
    return std::unexpected("Missing .mdl0shade definition");
  }
  return tmp;
}

Result<CrateAnimation> ReadCrateAnimation(const CrateAnimationPaths& paths) {
  auto mdl0mat = OishiiReadFile(paths.mdl0mat.string());
  if (!mdl0mat) {
    return std::unexpected(
        std::format("Failed to read .mdl0mat at \"{}\"", paths.mdl0mat));
  }
  auto mdl0shade = OishiiReadFile(paths.mdl0shade.string());
  if (!mdl0shade) {
    return std::unexpected(
        std::format("Failed to read .mdl0shade at \"{}\"", paths.mdl0shade));
  }
  std::vector<oishii::DataProvider> tex0s;
  for (auto& x : paths.tex0) {
    auto tex0 = OishiiReadFile(x.string());
    if (!tex0) {
      return std::unexpected(std::format("Failed to read .tex0 at \"{}\"", x));
    }
    tex0s.push_back(std::move(*tex0));
  }
  std::vector<oishii::DataProvider> srt0s;
  for (auto& x : paths.srt0) {
    auto srt0 = OishiiReadFile(x.string());
    if (!srt0) {
      return std::unexpected(std::format("Failed to read .srt0 at \"{}\"", x));
    }
    srt0s.push_back(std::move(*srt0));
  }
  auto mat = ReadMDL0Mat(mdl0mat->slice());
  if (!mat.has_value()) {
    return std::unexpected(std::format("Failed to parse material at \"{}\": {}",
                                       paths.mdl0mat, mat.error()));
  }
  auto shade = ReadMDL0Shade(mdl0shade->slice());
  if (!shade.has_value()) {
    return std::unexpected(std::format("Failed to parse shader at \"{}\": {}",
                                       paths.mdl0shade, shade.error()));
  }
  CrateAnimation tmp;
  for (size_t i = 0; i < tex0s.size(); ++i) {
    auto tex = ReadTEX0(tex0s[i].slice());
    if (!tex.has_value()) {
      if (i >= paths.tex0.size()) {
        return std::unexpected(tex.error());
      }
      return std::unexpected(std::format("Failed to parse TEX0 at \"{}\": {}",
                                         paths.tex0[i], tex.error()));
    }
    tmp.tex.push_back(*tex);
  }
  for (size_t i = 0; i < srt0s.size(); ++i) {
    auto srt = ReadSRT0(srt0s[i].slice());
    if (!srt.has_value()) {
      if (i >= paths.srt0.size()) {
        return std::unexpected(srt.error());
      }
      return std::unexpected(std::format("Failed to parse SRT0 at \"{}\": {}",
                                         paths.srt0[i], srt.error()));
    }
    tmp.srt.push_back(*srt);
  }

  {
    auto merged = ApplyG3dShaderToMaterial(*mat, *shade);
    if (!merged.has_value()) {
      return std::unexpected(std::format(
          "Failed to combine material + shader: {}", merged.error()));
    }
    tmp.mat = *merged;
  }
  tmp.mat.name = paths.preset_name;
  return tmp;
}

std::string RetargetCrateAnimation(CrateAnimation& preset) {
  std::set<std::string> mat_targets;
  for (const auto& x : preset.srt) {
    for (const auto& mat : x.materials) {
      mat_targets.emplace(mat.name);
    }
  }
  for (const auto& x : preset.clr) {
    for (const auto& mat : x.materials) {
      mat_targets.emplace(mat.name);
    }
  }
  for (const auto& x : preset.pat) {
    for (const auto& mat : x.materials) {
      mat_targets.emplace(mat.name);
    }
  }

  if (mat_targets.size() > 1) {
    // Until we get std::format w/ ranges in C++23
    const std::string srt_names =
        FormatRange(preset.srt, [](auto& x) { return x.name; });
    const std::string mat_names =
        FormatRange(mat_targets, [](auto& x) { return x; });

    return std::format(
        "Invalid single material preset. We expect one material per preset. "
        "Here, we scanned {} SRT0+CLR0+PAT0 files ({}) and saw references to "
        "more than just one material. In particular, the materials ({}) were "
        "referenced. It's not clear which SRT0/CLR0/PAT0 animations to keep "
        "and which to discard here, so the material preset is rejected as "
        "being invalid. Remove the extraneous data and try again.",
        preset.srt.size(), srt_names, mat_names);
  }

  // Map mat_targets[0] -> mat.name
  for (auto& x : preset.srt) {
    for (auto& mat : x.materials) {
      mat.name = preset.mat.name;
    }
  }
  for (auto& x : preset.clr) {
    for (auto& mat : x.materials) {
      mat.name = preset.mat.name;
    }
  }
  for (auto& x : preset.pat) {
    for (auto& mat : x.materials) {
      mat.name = preset.mat.name;
    }
  }

  return {};
}

struct Reader {
  oishii::DataProvider mData;
  oishii::BinaryReader mReader;

  Reader(std::string path, const std::vector<u8>& data)
      : mData(OishiiReadFile(path, data.data(), data.size())),
        mReader(mData.slice()) {}
};

struct SimpleTransaction {
  SimpleTransaction() {
    trans.callback = [](...) {};
  }

  bool success() const {
    return trans.state == kpi::TransactionState::Complete;
  }

  kpi::LightIOTransaction trans;
};

std::unique_ptr<librii::g3d::Archive> ReadBRRES(const std::vector<u8>& buf,
                                                std::string path) {
  SimpleTransaction trans;
  Reader reader(path, buf);
  librii::g3d::BinaryArchive bin;
  auto ok = bin.read(reader.mReader, trans.trans);
  if (!ok) {
    return nullptr;
  }
  if (!trans.success()) {
    return nullptr;
  }

  auto arc = librii::g3d::Archive::from(bin, trans.trans);
  if (!trans.success()) {
    return nullptr;
  }
  if (!arc) {
    return nullptr;
  }

  return std::make_unique<librii::g3d::Archive>(std::move(*arc));
}

Result<CrateAnimation> ReadRSPreset(std::span<const u8> file) {
  using namespace std::string_literals;

  std::vector<u8> buf(file.begin(), file.end());
  auto brres = ReadBRRES(buf, "<rs preset>");
  if (!brres) {
    return std::unexpected("Failed to parse .rspreset file"s);
  }
  if (brres->models.size() != 1) {
    return std::unexpected("Failed to parse .rspreset file"s);
  }
  auto& mdl = brres->models[0];
  if (mdl.materials.size() != 1) {
    return std::unexpected("Failed to parse .rspreset file"s);
  }
  auto mat = mdl.materials[0];
  for (auto& tex : mat.samplers) {
    if (findByName2(brres->textures, tex.mTexture) == nullptr) {
      return std::unexpected("Preset is missing texture "s + tex.mTexture);
    }
  }
  for (auto& srt : brres->srts) {
    for (auto& anim : srt.materials) {
      if (anim.name != mat.name) {
        return std::unexpected("Extraneous SRT0 animations included"s);
      }
    }
  }
  for (auto& clr : brres->clrs) {
    for (auto& anim : clr.materials) {
      if (anim.name != mat.name) {
        return std::unexpected("Extraneous CLR0 animations included"s);
      }
    }
  }
  for (auto& pat : brres->pats) {
    for (auto& anim : pat.materials) {
      if (anim.name != mat.name) {
        return std::unexpected("Extraneous PAT0 animations included"s);
      }
    }
  }

  const std::string s = mdl.bones.empty() ? "" : mdl.bones[0].mName;

  auto old_meta = s.substr(0, s.find("{BEGIN_STRUCTURED_DATA}"));
  auto new_meta = s.substr(old_meta.size());
  if (new_meta.size() > 23) {
    new_meta = new_meta.substr(23);
  }
  auto as_json = nlohmann::json::parse(new_meta, nullptr, false);
  if (as_json.is_discarded()) {
    fprintf(stderr, "Can't read json block");
    as_json = nlohmann::json();
  }

  return CrateAnimation{
      .mat = mat,
      .tex = brres->textures,
      .srt = brres->srts,
      .clr = brres->clrs,
      .pat = brres->pats,
      .metadata = old_meta,
      .metadata_json = as_json,
  };
}
Result<std::vector<u8>> WriteRSPreset(const CrateAnimation& preset) {
  librii::g3d::Archive collection;
  auto& mdl = collection.models.emplace_back();
  mdl.materials.push_back(preset.mat);
  collection.textures = preset.tex;
  collection.srts = preset.srt;
  collection.clrs = preset.clr;
  collection.pats = preset.pat;

  mdl.matrices.push_back(librii::g3d::DrawMatrix{.mWeights = {{0, 1.0f}}});

  auto json = preset.metadata_json;
  // Fill in date field
  const auto now = std::chrono::system_clock::now();
#ifndef __APPLE__
  json["date_created"] = std::format("{:%B %d, %Y}", now);
#endif
  json["tool"] = std::format("RiiStudio {}", GIT_TAG);

  // A bone is required for some reason
  mdl.bones.emplace_back().mName =
      preset.metadata + "{BEGIN_STRUCTURED_DATA}" + nlohmann::to_string(json);

  oishii::Writer writer(0);
  TRY(collection.binary()).write(writer);
  return writer.takeBuf();
}

Result<CrateAnimation> CreatePresetFromMaterial(const g3d::G3dMaterialData& mat,
                                                const g3d::Archive* scene,
                                                std::string_view metadata) {
  if (!scene) {
    return std::unexpected(
        "Internal: This scene type does not support .rsmat presets. "
        "Not a BRRES file?");
  }
  std::set<std::string> tex_names, srt_names;
  librii::crate::CrateAnimation result;
  result.mat = mat;
  result.metadata = metadata;
  if (metadata.empty()) {
    result.metadata =
        "RiiStudio " + std::string(RII_TIME_STAMP) + ";Source " + "scene->path";
  }
  for (auto& sampler : mat.samplers) {
    auto& tex = sampler.mTexture;
    if (tex_names.contains(tex)) {
      continue;
    }
    tex_names.emplace(tex);
    auto* data = findByName2(scene->textures, tex);
    if (data == nullptr) {
      return std::unexpected(
          std::string("Failed to find referenced textures ") + tex);
    }
    result.tex.push_back(*data);
  }
  for (auto& srt : scene->srts) {
    librii::g3d::SrtAnimationArchive mut = srt;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });
    if (!mut.materials.empty()) {
      result.srt.push_back(mut);
    }
  }
  for (auto& clr : scene->clrs) {
    librii::g3d::BinaryClr mut = clr;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });
    if (!mut.materials.empty()) {
      result.clr.push_back(mut);
    }
  }
  for (auto& pat : scene->pats) {
    librii::g3d::BinaryTexPat mut = pat;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });

    std::vector<u32> referenced;
    for (auto& mat : mut.materials) {
      for (auto& s : mat.samplers) {
        if (auto* u = std::get_if<u32>(&s)) {
          EXPECT(*u < mut.tracks.size());
          auto& track = mut.tracks[*u];
          for (auto& [idx, f] : track.keyframes) {
            EXPECT(f.palette == 0, "Palettes are not supported");
            EXPECT(f.texture < mut.textureNames.size());
            referenced.emplace_back(f.texture);
          }
        } else if (auto* f = std::get_if<librii::g3d::PAT0KeyFrame>(&s)) {
          EXPECT(f->palette == 0, "Palettes are not supported");
          referenced.emplace_back(f->texture);
        } else {
          return std::unexpected("Invalid PAT0");
        }
      }
    }

    for (auto u : referenced) {
      EXPECT(u < mut.textureNames.size());
      auto& tex = mut.textureNames[u];
      if (findByName2(result.tex, tex)) {
        continue;
      }
      auto* data = findByName2(scene->textures, tex);
      if (data == nullptr) {
        return std::unexpected(
            std::format("Failed to find referenced texture {}", tex));
      }
      result.tex.push_back(*data);
    }

    if (!mut.materials.empty()) {
      result.pat.push_back(mut);
    }
  }
  return result;
}

} // namespace librii::crate
