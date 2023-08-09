#include "j3d_crate.hpp"

#include <core/util/timestamp.hpp>
#include <plate/Platform.hpp>
#include <rsl/ArrayUtil.hpp>
#include <rsl/WriteFile.hpp>

namespace librii::crate {

struct SimpleTransaction {
  SimpleTransaction() {
    trans.callback = [](...) {};
  }

  kpi::LightIOTransaction trans;
};

[[nodiscard]] Result<CrateAnimationJ3D>
ReadRSPresetJ3D(std::span<const u8> file) {
  using namespace std::string_literals;
  SimpleTransaction trans;
  std::vector<u8> buf(file.begin(), file.end());
  auto reader = oishii::BinaryReader(buf, "<rs preset>", std::endian::big);
  auto bmd = librii::j3d::J3dModel::read(reader, trans.trans);
  if (!bmd) {
    return std::unexpected("Failed to parse .bmd_rspreset file"s);
  }
  if (bmd->materials.size() != 1) {
    return std::unexpected("Failed to parse .bmd_rspreset file"s);
  }
  auto mat = bmd->materials[0];
  for (auto& tex : mat.samplers) {
    if (findByName2(bmd->textures, tex.mTexture) == nullptr) {
      return std::unexpected("Preset is missing texture "s + tex.mTexture);
    }
  }

  const std::string s = bmd->joints.empty() ? "" : bmd->joints[0].name;

  auto as_json = nlohmann::json::parse(s, nullptr, false);
  if (as_json.is_discarded()) {
    rsl::error("Can't read json block");
    as_json = nlohmann::json();
  }

  return CrateAnimationJ3D{
      .mat = mat,
      .tex = bmd->textures,
      .metadata_json = as_json,
  };
}

[[nodiscard]] Result<std::vector<u8>>
WriteRSPresetJ3D(const CrateAnimationJ3D& preset, bool cli) {
  librii::j3d::J3dModel collection;
  collection.materials.push_back(preset.mat);
  collection.textures = preset.tex;

  collection.drawMatrices.push_back(
      libcube::DrawMatrix{.mWeights = {{0, 1.0f}}});

  auto json = preset.metadata_json;
  // Fill in date field
  const auto now = std::chrono::system_clock::now();
#if defined(_WIN32)
  json["date_created"] = std::format("{:%B %d, %Y}", now);
#endif
  json["tool"] = std::format("RiiStudio {}{}", cli ? "CLI " : "",
                             std::string_view(GIT_TAG));

  // A bone is required for some reason
  collection.joints.emplace_back().name = nlohmann::to_string(json);

  oishii::Writer writer(0, std::endian::big);
  TRY(collection.write(writer, false));
  return writer.takeBuf();
}

[[nodiscard]] Result<CrateAnimationJ3D>
CreatePresetFromMaterialJ3D(const j3d::MaterialData& mat,
                            const j3d::J3dModel* scene) {
  if (!scene) {
    return std::unexpected(
        "Internal: This scene type does not support .rspreset files. "
        "Not a BRRES file?");
  }
  std::set<std::string> tex_names, srt_names;
  librii::crate::CrateAnimationJ3D result;
  result.mat = mat;
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
  return result;
}

[[nodiscard]] Result<void> DumpPresetsToFolderJ3D(std::filesystem::path root,
                                                  const j3d::J3dModel& scene,
                                                  bool cli) {
  std::error_code ec;
  bool exists = std::filesystem::exists(root, ec);
  if (ec || !exists) {
    return std::unexpected("Error: output folder does not exist");
  }
  for (auto& mat : scene.materials) {
    auto preset = TRY(CreatePresetFromMaterialJ3D(mat, &scene));
    auto bytes = TRY(WriteRSPresetJ3D(preset, cli));
    auto dst = (root / (mat.name + ".bmd_rspreset")).string();
    rsl::WriteFile(bytes, dst);
  }
  return {};
}

} // namespace librii::crate
