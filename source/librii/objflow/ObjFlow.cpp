#include "ObjFlow.hpp"

#include <core/util/oishii.hpp>
#include <rsl/Ranges.hpp>
#include <rsl/SimpleReader.hpp>

namespace librii::objflow {

template <size_t L> struct CFixedString {
  std::string_view view() const { return {buf, strnlen(buf, sizeof(buf))}; }
  char buf[L]{};
};
static_assert(sizeof(CFixedString<256>) == 256);

struct RawObjectParameter {
  rsl::bs16 ID;
  CFixedString<32> Name;
  CFixedString<64> AssetsListing;
  rsl::bu16 P0_Sound;
  rsl::bu16 ClipSetting;
  rsl::bs16 VisualAABBSize;
  rsl::bs16 VisibilitySphereRadius;
  rsl::bs16 P4;
  rsl::bs16 P5;
  rsl::bs16 P6;
  rsl::bs16 P7;
  rsl::bs16 P8;
};
static_assert(sizeof(RawObjectParameter) == 0x74);

Result<ObjectParameters> Read(std::span<const u8> buf) {
  if (buf.size() < 2) {
    return std::unexpected("File is too small");
  }
  s16 entries = rsl::load<s16>(buf, 0);
  if (entries < 0) {
    return std::unexpected("Negative number of entries");
  }
  auto to_aend = 2 + entries * sizeof(RawObjectParameter);
  if (to_aend > buf.size()) {
    return std::unexpected("File is truncated");
  }
  if (entries == 0) {
    return {};
  }
  ObjectParameters result;
  result.parameters.resize(entries);

  s16 max = -1;

  for (size_t i = 0; i < result.parameters.size(); ++i) {
    auto subs = buf.subspan(2 + i * sizeof(RawObjectParameter));
    if (subs.size_bytes() < sizeof(RawObjectParameter)) {
      return std::unexpected("File is truncated");
    }
    RawObjectParameter param;
    std::memcpy(&param, subs.data(), sizeof(param));
    result.parameters[i] = {
        .ID = param.ID,
        .Name = std::string(param.Name.view()),
        .AssetsListing = std::string(param.AssetsListing.view()),
        .P0_Sound = param.P0_Sound,
        .ClipSetting = param.ClipSetting,
        .VisualAABBSize = param.VisualAABBSize,
        .VisibilitySphereRadius = param.VisibilitySphereRadius,
        .P4 = param.P4,
        .P5 = param.P5,
        .P6 = param.P6,
        .P7 = param.P7,
        .P8 = param.P8,
    };
    if (result.parameters[i].ID > max) {
      max = result.parameters[i].ID;
    }
  }

  if (max < 0) {
    return std::unexpected("Maximum object ID is negative");
  }
  const size_t count = max + 1;
  if (to_aend + 2 * count > buf.size()) {
    return std::unexpected("Truncated file: Missing remap table");
  }
  result.remap_table.resize(count);
  for (size_t i = 0; i < count; ++i) {
    result.remap_table[i] = rsl::load<s16>(buf, to_aend + 2 * i);
  }
  return result;
}

Result<ObjectParameters> ReadFromFile(std::string_view path) {
  auto f = ReadFile(path);
  if (!f) {
    return std::unexpected(std::format("Failed to read file {}", path));
  }
  return Read(*f);
}

std::string GetPrimaryResource(const ObjectParameter& param) {
  for (auto word : std::ranges::views::split(param.AssetsListing, ";")) {
    std::string_view sv(&*word.begin(), std::ranges::distance(word));
    if (sv != "-") {
      return std::string(sv);
	}
  }
  return "";
}

} // namespace librii::objflow
