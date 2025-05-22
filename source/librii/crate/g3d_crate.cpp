#include "g3d_crate.hpp"
#include <core/util/timestamp.hpp>
#include <librii/g3d/data/BoneData.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/ArchiveIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>
#include <rsl/ArrayUtil.hpp>
#include <rsl/WriteFile.hpp>

#include <rsl/Expected.hpp>
#define BRRES_SYS_NO_EXPECTED
#include <brres/lib/brres-sys/include/brres_sys.h>

#include <fmt/chrono.h>

namespace librii::crate {

Result<std::vector<u8>> WriteMDL0Mat(const g3d::G3dMaterialData& mat) {
  g3d::Model m;
  m.materials.push_back(mat);
  g3d::Archive a;
  a.models.push_back(m);
  auto d = g3d::DumpJson(a);
  return brres::brres_to_mdl0mat(d.jsonData, d.collatedBuffer);
}

std::vector<u8> WriteMDL0Shade(const g3d::G3dMaterialData& mat) {
  g3d::Model m;
  m.materials.push_back(mat);
  g3d::Archive a;
  a.models.push_back(m);
  auto d = g3d::DumpJson(a);
  auto res = brres::brres_to_mdl0shade(d.jsonData, d.collatedBuffer);
  assert(res);
  return *res;
}

Result<g3d::SrtAnimationArchive> ReadSRT0(std::span<const u8> file) {
  g3d::BinarySrt arc;
  oishii::BinaryReader reader(file, "Unknown SRT0", std::endian::big);
  auto ok = arc.read(reader);
  if (!ok) {
    return std::unexpected("Failed to parse SRT0: g3d::ReadSrtFile returned " +
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

Result<CrateAnimation> ReadCrateAnimation(std::filesystem::path path) {
  auto s = path.string();
  auto bytes = TRY(brres::read_mdl0mat_folder_preset(s));
  return ReadRSPreset(bytes);
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

    return std::unexpected(std::format(
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
    for (auto& anim : srt.matrices) {
      if (anim.target.materialName != mat.name) {
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
  json["date_created"] = fmt::format("{:%B %d, %Y}", now);
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
    return std::unexpected(
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
      return std::unexpected(
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
          return std::unexpected("Invalid PAT0");
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

[[nodiscard]] Result<void> DumpPresetsToFolder(std::filesystem::path root,
                                               const g3d::Archive& scene,
                                               bool cli,
                                               std::string_view metadata) {
  std::error_code ec;
  bool exists = std::filesystem::exists(root, ec);
  if (ec || !exists) {
    return std::unexpected("Error: output folder does not exist");
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
