#pragma once

#include <core/common.h>
#include <filesystem>
#include <librii/g3d/data/Archive.hpp>
#include <vendor/nlohmann/json.hpp>

namespace librii::g3d {
struct BinarySrt;
}

namespace librii::crate {

//! BrawlCrate-generated material definition. Does not include TEV/Swap/Indirect
//! Orders.
//!
//! mStages/mIndirectStages[...].indOrder will be set to default values; their
//! size will be valid, though.
//
[[nodiscard]] Result<g3d::G3dMaterialData>
ReadMDL0Mat(std::span<const u8> file);

[[nodiscard]] Result<std::vector<u8>>
WriteMDL0Mat(const g3d::G3dMaterialData& mat);

//! BrawlCrate-generated TEV definition. Includes Swap/Indirect Orders too.
//!
//! There will always be 4 indirectOrders provided; use the material's GEN_MODE
//! to know how many are actually used. Also, the stage count in the shader is
//! trusted here, although it may be unused by the game/runtime library.
//!
[[nodiscard]] Result<g3d::G3dShader> ReadMDL0Shade(std::span<const u8> file);

[[nodiscard]] std::vector<u8> WriteMDL0Shade(const g3d::G3dMaterialData& mat);

//! Applies the three fields in G3dShader to G3dMaterialData.
//!
//! If the material has 2 TEV stages already, it will only grab the first 2 TEV
//! stages of the shader. This is because a MDL0Mat could only use the first few
//! stages of a MDL0Shade. So here we just trust the material.
//!
//! Likewise, the number of indirectStages in the material determines the number
//! of indirectOrders pulled from the G3dShader.
//!
[[nodiscard]] Result<g3d::G3dMaterialData>
ApplyG3dShaderToMaterial(const g3d::G3dMaterialData& mat,
                         const g3d::G3dShader& tev);

//! A "TEX0" file is effectively a .brtex archive without the enclosing
//! structure.
//!
[[nodiscard]] Result<g3d::TextureData> ReadTEX0(std::span<const u8> file);

[[nodiscard]] std::vector<u8> WriteTEX0(const g3d::TextureData& tex);

//! A "SRT0" file is effectively a .brtsa archive without the enclosing
//! structure.
//!
[[nodiscard]] Result<g3d::SrtAnim> ReadSRT0(std::span<const u8> file);

[[nodiscard]] Result<std::vector<u8>> WriteSRT0(const g3d::BinarySrt& arc);

struct CrateAnimationPaths {
  std::string preset_name; // For sake of preset name
  std::filesystem::path mdl0mat;
  std::filesystem::path mdl0shade;
  std::vector<std::filesystem::path>
      tex0; // Should be .tex0; ignoring .png for now
  std::vector<std::filesystem::path> srt0;
};

struct CrateAnimation {
  g3d::G3dMaterialData mat;           // MDL0Mat + MDL0Shade combined
  std::vector<g3d::TextureData> tex;  // All valid .tex0s
  std::vector<g3d::SrtAnim> srt;      // All valid .srt archives
  std::vector<g3d::BinaryClr> clr;    // .clr0
  std::vector<g3d::BinaryTexPat> pat; // .pat0

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

[[nodiscard]] Result<CrateAnimationPaths>
ScanCrateAnimationFolder(std::filesystem::path path);
[[nodiscard]] Result<CrateAnimation>
ReadCrateAnimation(const CrateAnimationPaths& paths);

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
