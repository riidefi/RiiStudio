#pragma once

#include <core/common.h>
#include <filesystem>
#include <librii/g3d/data/Archive.hpp>
#include <vendor/nlohmann/json.hpp>

#include <librii/g3d/data/MaterialData.hpp>

namespace librii::g3d {
struct BinarySrt;
struct G3dShader;
} // namespace librii::g3d

namespace librii::crate {

[[nodiscard]] Result<std::vector<u8>>
WriteMDL0Mat(const g3d::G3dMaterialData& mat);

[[nodiscard]] std::vector<u8> WriteMDL0Shade(const g3d::G3dMaterialData& mat);

//! A "SRT0" file is effectively a .brtsa archive without the enclosing
//! structure.
//!
[[nodiscard]] Result<g3d::SrtAnim> ReadSRT0(std::span<const u8> file);

[[nodiscard]] Result<std::vector<u8>> WriteSRT0(const g3d::BinarySrt& arc);

struct CrateAnimation {
  g3d::G3dMaterialData mat;          // MDL0Mat + MDL0Shade combined
  std::vector<g3d::TextureData> tex; // All valid .tex0s
  std::vector<g3d::SrtAnim> srt;     // All valid .srt archives
  std::vector<g3d::ClrAnim> clr;     // .clr0
  std::vector<g3d::PatAnim> pat;     // .pat0

  std::string metadata;
  nlohmann::json metadata_json{
      {"author", "??"},
      {"date_created", "??"},
      {"comment", "??"},
      {"tool", "??"},
  };
};

inline std::optional<std::string> GetStringField(const CrateAnimation& crate,
                                                 const std::string& field) {
  if (!crate.metadata_json.contains(field)) {
    return std::nullopt;
  }
  if (!crate.metadata_json[field].is_string()) {
    return std::nullopt;
  }
  return crate.metadata_json[field].get<std::string>();
}

inline std::optional<std::string> GetAuthor(const CrateAnimation& crate) {
  return GetStringField(crate, "author");
}
inline std::optional<std::string> GetDateCreated(const CrateAnimation& crate) {
  return GetStringField(crate, "date_created");
}
inline std::optional<std::string> GetComment(const CrateAnimation& crate) {
  return GetStringField(crate, "comment");
}
inline std::optional<std::string> GetTool(const CrateAnimation& crate) {
  return GetStringField(crate, "tool");
}

[[nodiscard]] Result<CrateAnimation>
ReadCrateAnimation(std::filesystem::path path);

//! Animations targets materials by name; rename each SRT animation target to
//! the name of `mat`. We assert that only one material target is defined.
[[nodiscard]] Result<void> RetargetCrateAnimation(CrateAnimation& preset);

[[nodiscard]] Result<CrateAnimation> ReadRSPreset(std::span<const u8> file);

[[nodiscard]] Result<std::vector<u8>>
WriteRSPreset(const CrateAnimation& preset, bool cli = false);

[[nodiscard]] Result<CrateAnimation>
CreatePresetFromMaterial(const g3d::G3dMaterialData& mat,
                         const g3d::Archive* scene, std::string_view metadata);

[[nodiscard]] Result<void> DumpPresetsToFolder(std::filesystem::path root,
                                               const g3d::Archive& scene,
                                               bool cli,
                                               std::string_view metadata);

} // namespace librii::crate
