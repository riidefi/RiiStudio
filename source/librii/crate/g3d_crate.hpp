#pragma once

#include <filesystem>
#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <librii/g3d/data/TextureData.hpp>
#include <rsl/Expected.hpp>
#include <variant>

namespace librii::crate {

//! BrawlCrate-generated material definition. Does not include TEV/Swap/Indirect
//! Orders.
//!
//! mStages/mIndirectStages[...].indOrder will be set to default values; their
//! size will be valid, though.
//
rsl::expected<g3d::G3dMaterialData, std::string>
ReadMDL0Mat(std::span<const u8> file);

//! BrawlCrate-generated TEV definition. Includes Swap/Indirect Orders too.
//!
//! There will always be 4 indirectOrders provided; use the material's GEN_MODE
//! to know how many are actually used. Also, the stage count in the shader is
//! trusted here, although it may be unused by the game/runtime library.
//!
rsl::expected<g3d::G3dShader, std::string>
ReadMDL0Shade(std::span<const u8> file);

//! Applies the three fields in G3dShader to G3dMaterialData.
//!
//! If the material has 2 TEV stages already, it will only grab the first 2 TEV
//! stages of the shader. This is because a MDL0Mat could only use the first few
//! stages of a MDL0Shade. So here we just trust the material.
//!
//! Likewise, the number of indirectStages in the material determines the number
//! of indirectOrders pulled from the G3dShader.
//!
rsl::expected<g3d::G3dMaterialData, std::string>
ApplyG3dShaderToMaterial(const g3d::G3dMaterialData& mat,
                         const g3d::G3dShader& tev);

//! A "TEX0" file is effectively a .brtex archive without the enclosing
//! structure.
//!
rsl::expected<g3d::TextureData, std::string> ReadTEX0(std::span<const u8> file);

std::vector<u8> WriteTEX0(const g3d::TextureData& tex);

//! A "SRT0" file is effectively a .brtsa archive without the enclosing
//! structure.
//!
rsl::expected<g3d::SrtAnimationArchive, std::string>
ReadSRT0(std::span<const u8> file);

std::vector<u8> WriteSRT0(const g3d::SrtAnimationArchive& arc);

struct CrateAnimationPaths {
  std::string preset_name; // For sake of preset name
  std::filesystem::path mdl0mat;
  std::filesystem::path mdl0shade;
  std::vector<std::filesystem::path>
      tex0; // Should be .tex0; ignoring .png for now
  std::vector<std::filesystem::path> srt0;
};

struct CrateAnimation {
  g3d::G3dMaterialData mat;                  // MDL0Mat + MDL0Shade combined
  std::vector<g3d::TextureData> tex;         // All valid .tex0s
  std::vector<g3d::SrtAnimationArchive> srt; // All valid .srt archives
};

rsl::expected<CrateAnimationPaths, std::string>
ScanCrateAnimationFolder(std::filesystem::path path);
rsl::expected<CrateAnimation, std::string>
ReadCrateAnimation(const CrateAnimationPaths& paths);

//! Animations targets materials by name; rename each SRT animation target to
//! the name of `mat`. We assert that only one material target is defined.
std::string RetargetCrateAnimation(CrateAnimation& preset);

} // namespace librii::crate
