#pragma once

#include <filesystem>
#include <librii/j3d/J3dIo.hpp>
#include <vendor/nlohmann/json.hpp>

namespace librii::crate {

struct CrateAnimationJ3D {
  j3d::MaterialData mat;             // MDL0Mat + MDL0Shade combined
  std::vector<j3d::TextureData> tex; // All valid .tex0s

  nlohmann::json metadata_json{
      {"author", "??"},
      {"date_created", "??"},
      {"comment", "??"},
      {"tool", "??"},
  };
};

inline std::optional<std::string> GetStringField(const CrateAnimationJ3D& crate,
                                                 const std::string& field) {
  if (!crate.metadata_json.contains(field)) {
    return std::nullopt;
  }
  if (!crate.metadata_json[field].is_string()) {
    return std::nullopt;
  }
  return crate.metadata_json[field].get<std::string>();
}

inline std::optional<std::string> GetAuthor(const CrateAnimationJ3D& crate) {
  return GetStringField(crate, "author");
}
inline std::optional<std::string>
GetDateCreated(const CrateAnimationJ3D& crate) {
  return GetStringField(crate, "date_created");
}
inline std::optional<std::string> GetComment(const CrateAnimationJ3D& crate) {
  return GetStringField(crate, "comment");
}
inline std::optional<std::string> GetTool(const CrateAnimationJ3D& crate) {
  return GetStringField(crate, "tool");
}

[[nodiscard]] Result<CrateAnimationJ3D>
ReadRSPresetJ3D(std::span<const u8> file);

[[nodiscard]] Result<std::vector<u8>>
WriteRSPresetJ3D(const CrateAnimationJ3D& preset, bool cli = false);

[[nodiscard]] Result<CrateAnimationJ3D>
CreatePresetFromMaterialJ3D(const j3d::MaterialData& mat,
                            const j3d::J3dModel* scene);

[[nodiscard]] Result<void> DumpPresetsToFolderJ3D(std::filesystem::path root,
                                                  const j3d::J3dModel& scene,
                                                  bool cli);

} // namespace librii::crate
