#pragma once

#include <core/common.h>

namespace librii::objflow {

struct ObjectParameter {
  s16 ID;
  // Max size: 32
  std::string Name;
  // Max size: 64
  std::string AssetsListing;
  u16 P0_Sound;
  u16 ClipSetting;
  s16 VisualAABBSize;
  s16 VisibilitySphereRadius;
  s16 P4;
  s16 P5;
  s16 P6;
  s16 P7;
  s16 P8;
};

struct ObjectParameters {
  std::vector<ObjectParameter> parameters;

  // Kuribo(401) -> index in parameters
  std::vector<s16> remap_table;
};

Result<ObjectParameters> Read(std::span<const u8> buf);
Result<ObjectParameters> ReadFromFile(std::string_view path);

std::string GetPrimaryResource(const ObjectParameter& param);

} // namespace librii::objflow
