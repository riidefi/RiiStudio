#include "g3d_crate.hpp"
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>
#include <set>

namespace librii::crate {

std::variant<g3d::G3dMaterialData, std::string>
ReadMDL0Mat(std::span<const u8> file) {
  std::vector<u8> bruh;
  oishii::DataProvider provider(std::move(bruh));
  oishii::ByteView view(file, provider, "MDL0Mat");
  oishii::BinaryReader reader(std::move(view));

  g3d::G3dMaterialData mat;
  const bool ok = g3d::readMaterial(mat, reader, /* ignore_tev */ true);
  if (!ok) {
    return "Failed to read MDL0Mat file";
  }

  return mat;
}

std::variant<g3d::G3dShader, std::string>
ReadMDL0Shade(std::span<const u8> file) {
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

std::variant<g3d::TextureData, std::string> ReadTEX0(std::span<const u8> file) {
  g3d::TextureData tex;
  // TODO: We trust the .tex0-file provided name to be correct
  const bool ok = g3d::ReadTexture(tex, file, "");
  if (!ok) {
    return "Failed to parse";
  }
  return tex;
}

std::variant<g3d::SrtAnimationArchive, std::string>
ReadSRT0(std::span<const u8> file) {
  // Good API!
  g3d::SrtAnimationArchive arc;
  const bool ok = g3d::ReadSrtFile(arc, file);
  if (!ok) {
    return "Failed to parse";
  }
  return arc;
}

std::variant<g3d::G3dMaterialData, std::string>
ApplyG3dShaderToMaterial(const g3d::G3dMaterialData& mat,
                         const g3d::G3dShader& tev) {
  const size_t num_istage = mat.indirectStages.size();
  assert(tev.mIndirectOrders.size() >= num_istage &&
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

std::variant<CrateAnimationPaths, std::string>
ScanCrateAnimationFolder(std::filesystem::path path) {
  assert(std::filesystem::is_directory(path));
  CrateAnimationPaths tmp;
  tmp.preset_name = path.stem().string();
  for (const auto& entry : std::filesystem::directory_iterator{path}) {
    const auto extension = entry.path().extension();
    if (extension == ".mdl0mat") {
      if (!tmp.mdl0mat.empty()) {
        return "Rejecting: Multiple .mdl0mat files in folder";
      }
      tmp.mdl0mat = entry.path();
    } else if (extension == ".mdl0shade") {
      if (!tmp.mdl0shade.empty()) {
        return "Rejecting: Multiple .mdl0shade files in folder";
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
    return "Missing .mdl0mat definition";
  }
  if (tmp.mdl0shade.empty()) {
    return "Missing .mdl0shade definition";
  }
  return tmp;
}

std::variant<CrateAnimation, std::string>
ReadCrateAnimation(const CrateAnimationPaths& paths) {
  auto mdl0mat = OishiiReadFile(paths.mdl0mat.string());
  if (!mdl0mat) {
    return "Failed to read .mdl0mat at \"" + paths.mdl0mat.string() + "\"";
  }
  auto mdl0shade = OishiiReadFile(paths.mdl0shade.string());
  if (!mdl0shade) {
    return "Failed to read .mdl0shade at \"" + paths.mdl0shade.string() + "\"";
  }
  std::vector<oishii::DataProvider> tex0s;
  for (auto& x : paths.tex0) {
    auto tex0 = OishiiReadFile(x.string());
    if (!tex0) {
      return "Failed to read .tex0 at \"" + x.string() + "\"";
    }
    tex0s.push_back(std::move(*tex0));
  }
  std::vector<oishii::DataProvider> srt0s;
  for (auto& x : paths.srt0) {
    auto srt0 = OishiiReadFile(x.string());
    if (!srt0) {
      return "Failed to read .srt0 at \"" + x.string() + "\"";
    }
    srt0s.push_back(std::move(*srt0));
  }
  auto mat = ReadMDL0Mat(mdl0mat->slice());
  if (auto* err = std::get_if<std::string>(&mat); err != nullptr) {
    return "Failed to parse material at \"" + paths.mdl0mat.string() +
           "\": " + *err;
  }
  auto shade = ReadMDL0Shade(mdl0shade->slice());
  if (auto* err = std::get_if<std::string>(&shade); err != nullptr) {
    return "Failed to parse shader at \"" + paths.mdl0shade.string() +
           "\": " + *err;
  }
  CrateAnimation tmp;
  for (size_t i = 0; i < tex0s.size(); ++i) {
    auto tex = ReadTEX0(tex0s[i].slice());
    if (auto* err = std::get_if<std::string>(&tex); err != nullptr) {
      if (i >= paths.tex0.size()) {
        return *err;
      }
      return "Failed to parse TEX0 at \"" + paths.tex0[i].string() +
             "\": " + *err;
    }
    tmp.tex.push_back(*std::get_if<g3d::TextureData>(&tex));
  }
  for (size_t i = 0; i < srt0s.size(); ++i) {
    auto srt = ReadSRT0(srt0s[i].slice());
    if (auto* err = std::get_if<std::string>(&srt); err != nullptr) {
      if (i >= paths.srt0.size()) {
        return *err;
      }
      return "Failed to parse SRT0 at \"" + paths.srt0[i].string() +
             "\": " + *err;
    }
    tmp.srt.push_back(*std::get_if<g3d::SrtAnimationArchive>(&srt));
  }

  auto raw_mat = *std::get_if<g3d::G3dMaterialData>(&mat);
  auto raw_shade = *std::get_if<g3d::G3dShader>(&shade);

  auto merged = ApplyG3dShaderToMaterial(raw_mat, raw_shade);
  if (auto* err = std::get_if<std::string>(&merged); err != nullptr) {
    return "Failed to combine material + shader: " + *err;
  }
  tmp.mat = *std::get_if<g3d::G3dMaterialData>(&merged);
  tmp.mat.name = paths.preset_name;
  return tmp;
}

std::string RetargetCrateAnimation(CrateAnimation& preset) {
  std::set<std::string> mat_targets;
  for (const auto& srt : preset.srt) {
    for (const auto& mat : srt.mat_animations) {
      mat_targets.emplace(mat.material_name);
    }
  }

  if (mat_targets.size() > 1) {
    // Until we get std::format w/ ranges in C++23
    const std::string srt_names =
        FormatRange(preset.srt, [](auto& x) { return x.name; });
    const std::string mat_names =
        FormatRange(mat_targets, [](auto& x) { return x; });

    return "Invalid single material preset. We expect one material per preset. "
           "Here, we scanned " +
           std::to_string(preset.srt.size()) + " SRT0 files (" + srt_names +
           ") and saw references to more than "
           "just one material. In particular, the materials (" +
           mat_names +
           ") were referenced. "
           "It's not clear which SRT0 animations to keep and which to discard "
           "here, so the material preset is rejected as being invalid.";
  }

  // Map mat_targets[0] -> mat.name
  for (auto& srt : preset.srt) {
    for (auto& mat : srt.mat_animations) {
      mat.material_name = preset.mat.name;
    }
  }

  return {};
}

} // namespace librii::crate
