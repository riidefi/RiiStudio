#include "g3d_crate.hpp"
#include <core/util/timestamp.hpp>
#include <librii/g3d/data/BoneData.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/ArchiveIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>
#include <rsl/ArrayUtil.hpp>
#include <rsl/WriteFile.hpp>

const char GIT_TAG[] __attribute__((weak)) = "`brres` pre-release";
const char RII_TIME_STAMP[] __attribute__((weak)) = "`brres` pre-release";

namespace librii::crate {

Result<g3d::G3dMaterialData> ReadMDL0Mat(std::span<const u8> file) {
  oishii::BinaryReader reader(file, "Unknown MDL0Mat", std::endian::big);

  g3d::G3dMaterialData mat;
  const bool ok = g3d::readMaterial(mat, reader, /* ignore_tev */ true);
  if (!ok) {
    return RSL_UNEXPECTED("Failed to read MDL0Mat file");
  }

  return mat;
}

Result<std::vector<u8>> WriteMDL0Mat(const g3d::G3dMaterialData& mat) {
  oishii::Writer writer(std::endian::big);

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

  oishii::BinaryReader reader(file, "Unknown MDL0Shade", std::endian::big);

  // This function is a bit more complex, as we normally assume materials are
  // read before shaders; here we don't make such assumptions.

  gx::LowLevelGxMaterial gx_mat;
  // So the DL parser doesn't discard IndOrders
  gx_mat.indirectStages.resize(4);
  // Trust the stage count since we have not necessarily read the material yet.
  TRY(g3d::ReadTev(gx_mat, reader, /* tev_addr */ 0,
                   /* trust_stagecount */ true, /* brawlbox_bug */ true));

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
  oishii::Writer writer(std::endian::big);

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

Result<g3d::SrtAnimationArchive> ReadSRT0(std::span<const u8> file) {
  g3d::BinarySrt arc;
  oishii::BinaryReader reader(file, "Unknown SRT0", std::endian::big);
  auto ok = arc.read(reader);
  if (!ok) {
    return RSL_UNEXPECTED("Failed to parse SRT0: g3d::ReadSrtFile returned " +
                           ok.error());
  }
  return g3d::SrtAnim::read(arc, [](...) {});
}

Result<std::vector<u8>> WriteSRT0(const g3d::BinarySrt& arc) {
  oishii::Writer writer(std::endian::big);

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
        return RSL_UNEXPECTED("Rejecting: Multiple .mdl0mat files in folder");
      }
      tmp.mdl0mat = entry.path();
    } else if (extension == ".mdl0shade") {
      if (!tmp.mdl0shade.empty()) {
        return RSL_UNEXPECTED(
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
    return RSL_UNEXPECTED("Missing .mdl0mat definition");
  }
  if (tmp.mdl0shade.empty()) {
    return RSL_UNEXPECTED("Missing .mdl0shade definition");
  }
  return tmp;
}

Result<CrateAnimation> ReadCrateAnimation(const CrateAnimationPaths& paths) {
  auto mdl0mat = OishiiReadFile2(paths.mdl0mat.string());
  if (!mdl0mat) {
    return RSL_UNEXPECTED(std::format("Failed to read .mdl0mat at \"{}\"",
                                       paths.mdl0mat.string()));
  }
  auto mdl0shade = OishiiReadFile2(paths.mdl0shade.string());
  if (!mdl0shade) {
    return RSL_UNEXPECTED(std::format("Failed to read .mdl0shade at \"{}\"",
                                       paths.mdl0shade.string()));
  }
  std::vector<std::vector<u8>> tex0s;
  for (auto& x : paths.tex0) {
    auto tex0 = OishiiReadFile2(x.string());
    if (!tex0) {
      return RSL_UNEXPECTED(
          std::format("Failed to read .tex0 at \"{}\"", x.string()));
    }
    tex0s.push_back(std::move(*tex0));
  }
  std::vector<std::vector<u8>> srt0s;
  for (auto& x : paths.srt0) {
    auto srt0 = OishiiReadFile2(x.string());
    if (!srt0) {
      return RSL_UNEXPECTED(
          std::format("Failed to read .srt0 at \"{}\"", x.string()));
    }
    srt0s.push_back(std::move(*srt0));
  }
  auto mat = ReadMDL0Mat(*mdl0mat);
  if (!mat.has_value()) {
    return RSL_UNEXPECTED(std::format("Failed to parse material at \"{}\": {}",
                                       paths.mdl0mat.string(), mat.error()));
  }
  auto shade = ReadMDL0Shade(*mdl0shade);
  if (!shade.has_value()) {
    return RSL_UNEXPECTED(std::format("Failed to parse shader at \"{}\": {}",
                                       paths.mdl0shade.string(),
                                       shade.error()));
  }
  CrateAnimation tmp;
  for (size_t i = 0; i < tex0s.size(); ++i) {
    auto tex = g3d::ReadTEX0(tex0s[i]);
    if (!tex.has_value()) {
      if (i >= paths.tex0.size()) {
        return RSL_UNEXPECTED(tex.error());
      }
      return RSL_UNEXPECTED(std::format("Failed to parse TEX0 at \"{}\": {}",
                                         paths.tex0[i].string(), tex.error()));
    }
    tmp.tex.push_back(*tex);
  }
  for (size_t i = 0; i < srt0s.size(); ++i) {
    auto srt = ReadSRT0(srt0s[i]);
    if (!srt.has_value()) {
      if (i >= paths.srt0.size()) {
        return RSL_UNEXPECTED(srt.error());
      }
      return RSL_UNEXPECTED(std::format("Failed to parse SRT0 at \"{}\": {}",
                                         paths.srt0[i].string(), srt.error()));
    }
    tmp.srt.push_back(*srt);
  }

  {
    auto merged = ApplyG3dShaderToMaterial(*mat, *shade);
    if (!merged.has_value()) {
      return RSL_UNEXPECTED(std::format(
          "Failed to combine material + shader: {}", merged.error()));
    }
    tmp.mat = *merged;
  }
  tmp.mat.name = paths.preset_name;
  return tmp;
}

Result<void> RetargetCrateAnimation(CrateAnimation& preset) {
  std::set<std::string> mat_targets;
  for (const auto& x : preset.srt) {
    for (const auto& mat : x.matrices) {
      mat_targets.emplace(mat.target.materialName);
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

    return RSL_UNEXPECTED(std::format(
        "Invalid single material preset. We expect one material per preset. "
        "Here, we scanned {} SRT0+CLR0+PAT0 files ({}) and saw references to "
        "more than just one material. In particular, the materials ({}) were "
        "referenced. It's not clear which SRT0/CLR0/PAT0 animations to keep "
        "and which to discard here, so the material preset is rejected as "
        "being invalid. Remove the extraneous data and try again.",
        preset.srt.size(), srt_names, mat_names));
  }

  // Map mat_targets[0] -> mat.name
  for (auto& x : preset.srt) {
    auto w = x.write(x);
    for (auto& mat : w.materials) {
      mat.name = preset.mat.name;
    }
    x = TRY(x.read(w, [](...) {}));
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

std::unique_ptr<librii::g3d::Archive> ReadBRRES(const std::vector<u8>& buf,
                                                std::string path) {
  auto arc = librii::g3d::Archive::fromMemory(buf, path);
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
    return RSL_UNEXPECTED("Failed to parse .rspreset file"s);
  }
  if (brres->models.size() != 1) {
    return RSL_UNEXPECTED("Failed to parse .rspreset file"s);
  }
  auto& mdl = brres->models[0];
  if (mdl.materials.size() != 1) {
    return RSL_UNEXPECTED("Failed to parse .rspreset file"s);
  }
  auto mat = mdl.materials[0];
  for (auto& tex : mat.samplers) {
    if (findByName2(brres->textures, tex.mTexture) == nullptr) {
      return RSL_UNEXPECTED("Preset is missing texture "s + tex.mTexture);
    }
  }
  for (auto& srt : brres->srts) {
    for (auto& anim : srt.matrices) {
      if (anim.target.materialName != mat.name) {
        return RSL_UNEXPECTED("Extraneous SRT0 animations included"s);
      }
    }
  }
  for (auto& clr : brres->clrs) {
    for (auto& anim : clr.materials) {
      if (anim.name != mat.name) {
        return RSL_UNEXPECTED("Extraneous CLR0 animations included"s);
      }
    }
  }
  for (auto& pat : brres->pats) {
    for (auto& anim : pat.materials) {
      if (anim.name != mat.name) {
        return RSL_UNEXPECTED("Extraneous PAT0 animations included"s);
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
    rsl::error("Can't read json block");
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
Result<std::vector<u8>> WriteRSPreset(const CrateAnimation& preset, bool cli) {
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
#if defined(_WIN32)
  json["date_created"] = std::format("{:%B %d, %Y}", now);
#endif
  json["tool"] = std::format("RiiStudio {}{}", cli ? "CLI " : "",
                             std::string_view(GIT_TAG));

  // A bone is required for some reason
  mdl.bones.emplace_back().mName =
      preset.metadata + "{BEGIN_STRUCTURED_DATA}" + nlohmann::to_string(json);

  return collection.write();
}

Result<CrateAnimation> CreatePresetFromMaterial(const g3d::G3dMaterialData& mat,
                                                const g3d::Archive* scene,
                                                std::string_view metadata) {
  if (!scene) {
    return RSL_UNEXPECTED(
        "Internal: This scene type does not support .rspreset files. "
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
      return RSL_UNEXPECTED(
          std::string("Failed to find referenced textures ") + tex);
    }
    result.tex.push_back(*data);
  }
  for (auto& srt : scene->srts) {
    librii::g3d::BinarySrt mut = srt.write(srt);
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });
    if (!mut.materials.empty()) {
      result.srt.push_back(TRY(librii::g3d::SrtAnim::read(mut, [](...) {})));
    }
  }
  for (auto& clr : scene->clrs) {
    librii::g3d::ClrAnim mut = clr;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });
    if (!mut.materials.empty()) {
      result.clr.push_back(mut);
    }
  }
  for (auto& pat : scene->pats) {
    librii::g3d::PatAnim mut = pat;
    std::erase_if(mut.materials, [&](auto& m) { return m.name != mat.name; });

    std::vector<u32> referenced;
    for (auto& mat : mut.materials) {
      for (auto& s : mat.samplers) {
#if 0
        if (auto* u = std::get_if<u32>(&s)) {
#else
        std::optional<u32> u = s;
#endif
        EXPECT(*u < mut.tracks.size());
        auto& track = mut.tracks[*u];
        for (auto& [idx, f] : track.keyframes) {
          // For now, force palette to 0 as BrawlBox sets it to some other
          // value
          f.palette = 0;
          EXPECT(f.palette == 0, "PAT0: Palettes are not supported; each "
                                 "keyframe should have paletteIdx of 0");
          EXPECT(f.texture < mut.textureNames.size());
          referenced.emplace_back(f.texture);
        }
#if 0
        }
		else if (auto* f = std::get_if<librii::g3d::PAT0KeyFrame>(&s)) {
          // For now, force palette to 0 as BrawlBox sets it to some other value
          f->palette = 0;
          EXPECT(f->palette == 0, "PAT0: Palettes are not supported; each "
                                  "keyframe should have paletteIdx of 0");
          referenced.emplace_back(f->texture);
        } else {
          return RSL_UNEXPECTED("Invalid PAT0");
        }
#endif
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
        return RSL_UNEXPECTED(
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

[[nodiscard]] Result<void> DumpPresetsToFolder(std::filesystem::path root,
                                               const g3d::Archive& scene,
                                               bool cli,
                                               std::string_view metadata) {
  std::error_code ec;
  bool exists = std::filesystem::exists(root, ec);
  if (ec || !exists) {
    return RSL_UNEXPECTED("Error: output folder does not exist");
  }
  for (auto& mdl : scene.models) {
    for (auto& mat : mdl.materials) {
      auto preset = TRY(CreatePresetFromMaterial(mat, &scene, metadata));
      auto bytes = TRY(WriteRSPreset(preset, cli));
      auto dst = (root / (mat.name + ".rspreset")).string();
      TRY(rsl::WriteFile(bytes, dst));
    }
  }
  return {};
}

} // namespace librii::crate
