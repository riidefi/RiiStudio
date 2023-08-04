#include "Model.hpp"
#include <algorithm>
#include <math.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace glm {
static inline void to_json(json& j, const vec3& v) { j = json{v.x, v.y, v.z}; }
static inline void from_json(const json& j, vec3& v) {
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
  j.at(2).get_to(v.z);
}
} // namespace glm

namespace librii::kcol {

//! Sizes, in bytes
struct SectionSizes {
  const u32 pos_data_size = 0;
  const u32 nrm_data_size = 0;
  const u32 prism_data_size = 0;
  const u32 block_data_size = 0;
};

static SectionSizes GetSectionSizes(const KCollisionV1Header& header,
                                    u32 file_size) {
  /*
   0 pos_data_offset
   1 nrm_data_offset
   2 prism_data_offset
   3 block_data_offset
   4 <end_of_file>
  */
  struct SectionEntry {
    u32 offset = 0;
    size_t index = 0;
    u32 size = 0;
  };

  // prism_data_offset is 1-indexed, so the offset is pulled back
  std::array<SectionEntry, 5> sections{
      SectionEntry{.offset = header.pos_data_offset, .index = 0},
      SectionEntry{.offset = header.nrm_data_offset, .index = 1},
      SectionEntry{.offset = static_cast<u32>(header.prism_data_offset +
                                              sizeof(KCollisionPrismData)),
                   .index = 2},
      SectionEntry{.offset = header.block_data_offset, .index = 3},
      SectionEntry{.offset = file_size, .index = 4}};

  std::sort(sections.begin(), sections.end(),
            [](auto& lhs, auto& rhs) { return lhs.offset < rhs.offset; });

  for (size_t i = 0; i < 4; ++i) {
    sections[i].size = sections[i + 1].offset - sections[i].offset;
  }

  std::sort(sections.begin(), sections.end(),
            [](auto& lhs, auto& rhs) { return lhs.index < rhs.index; });

  const u32 pos_data_size = sections[0].size;
  const u32 nrm_data_size = sections[1].size;
  const u32 prism_data_size = sections[2].size;
  const u32 block_data_size = sections[3].size;

  return {pos_data_size, nrm_data_size, prism_data_size, block_data_size};
}

Result<KCollisionData> ReadKCollisionData(std::span<const u8> bytes,
                                          u32 file_size) {
  KCollisionData data;
  if (bytes.size_bytes() < file_size) {
    return std::unexpected("Invalid file size");
  }

  auto* header = rsl::buffer_cast<const KCollisionV1Header>(bytes);
  if (header == nullptr) {
    return std::unexpected("Data is not large enough to hold header");
  }

  data = KCollisionData{.prism_thickness = header->prism_thickness,
                        .area_min_pos = glm::vec3{header->area_min_pos.x,
                                                  header->area_min_pos.y,
                                                  header->area_min_pos.z},

                        .area_x_width_mask = header->area_x_width_mask,
                        .area_y_width_mask = header->area_y_width_mask,
                        .area_z_width_mask = header->area_z_width_mask,

                        .block_width_shift = header->block_width_shift,
                        .area_x_blocks_shift = header->area_x_blocks_shift,
                        .area_xy_blocks_shift = header->area_xy_blocks_shift,

                        .sphere_radius = header->sphere_radius};

  {
    const auto metadata = InspectKclFile(bytes.subspan(0, file_size));
    data.version = GetKCLVersion(metadata);
  }

  const auto sizes = GetSectionSizes(*header, file_size);

  data.pos_data.resize(sizes.pos_data_size / sizeof(Vector3f));
  for (size_t i = 0; i < data.pos_data.size(); ++i) {
    auto* pos = rsl::buffer_cast<const Vector3f>(
        bytes, header->pos_data_offset + i * sizeof(Vector3f));
    if (pos == nullptr) {
      return std::unexpected("Bug in reading code");
    }

    data.pos_data[i] = glm::vec3{pos->x, pos->y, pos->z};
  }

  data.nrm_data.resize(sizes.nrm_data_size / sizeof(Vector3f));
  for (size_t i = 0; i < data.nrm_data.size(); ++i) {
    auto* nrm = rsl::buffer_cast<const Vector3f>(
        bytes, header->nrm_data_offset + i * sizeof(Vector3f));
    if (nrm == nullptr) {
      return std::unexpected("Bug in reading code");
    }

    data.nrm_data[i] = glm::vec3{nrm->x, nrm->y, nrm->z};
  }

  data.prism_data.resize(sizes.prism_data_size / sizeof(KCollisionPrismData));
  for (size_t i = 0; i < data.prism_data.size(); ++i) {
    auto* prism = rsl::buffer_cast<const KCollisionPrismData>(
        bytes,
        header->prism_data_offset + (i + 1) * sizeof(KCollisionPrismData));
    if (prism == nullptr) {
      return std::unexpected("Bug in reading code");
    }

    data.prism_data[i] = *prism;
  }

  if (header->block_data_offset + sizes.block_data_size > file_size) {
    return std::unexpected("Bug in reading code");
  }

  data.block_data = {bytes.data() + header->block_data_offset,
                     bytes.data() + header->block_data_offset +
                         sizes.block_data_size};

  return data;
}

constexpr std::array<char, 8> WiimmSZSIdentifier = {'W', 'i', 'i', 'm',
                                                    'm', 'S', 'Z', 'S'};

//! The first ~~normal?~~ position will be this type
struct WiimmKclMetadata {
  std::array<char, 8> identifier = WiimmSZSIdentifier;
  rsl::bf32 version = 0.0f;
};

static_assert(sizeof(WiimmKclMetadata) == sizeof(glm::vec3));

KclVersion InspectKclFile(std::span<const u8> kcl_file) {
  auto* header = rsl::buffer_cast<const KCollisionV1Header>(kcl_file);
  if (header == nullptr) {
    // Data is not large enough
    return InvalidKclVersion{};
  }

  const auto sizes = GetSectionSizes(*header, kcl_file.size_bytes());

  if (sizes.pos_data_size < sizeof(glm::vec3)) {
    // File is empty
    return UnknownKclVersion{};
  }

  static_assert(sizeof(WiimmKclMetadata) == sizeof(glm::vec3));
  auto* wiimm_metadata = rsl::buffer_cast<const WiimmKclMetadata>(
      kcl_file, header->pos_data_offset);

  if (wiimm_metadata == nullptr) {
    // This shouldnt be reached
    return InvalidKclVersion{};
  }

  if (wiimm_metadata->identifier != WiimmSZSIdentifier) {
    // No other heuristics for now
    return UnknownKclVersion{};
  }

  const int major_version = static_cast<int>(wiimm_metadata->version);
  const int minor_version =
      static_cast<int>(fmodf(wiimm_metadata->version, 1.0f) * 100.0f);

  return WiimmKclVersion{.major_version = major_version,
                         .minor_version = minor_version};
}

std::string GetKCLVersion(KclVersion metadata) {
  if (auto* as_wiimm = std::get_if<librii::kcol::WiimmKclVersion>(&metadata)) {
    return FormatWiimmKclVersion(*as_wiimm);
  } else if (auto* as_unknown =
                 std::get_if<librii::kcol::UnknownKclVersion>(&metadata)) {
    return FormatUnknownKclVersion(*as_unknown);
  } else if (auto* as_invalid =
                 std::get_if<librii::kcol::InvalidKclVersion>(&metadata)) {
    return FormatInvalidKclVersion(*as_invalid);
  }
  return "";
}

void to_json(json& j, const KCollisionPrismData& p) {
  j = json{{"height", *p.height},      {"pos_i", *p.pos_i},
           {"fnrm_i", *p.fnrm_i},      {"enrm1_i", *p.enrm1_i},
           {"enrm2_i", *p.enrm2_i},    {"enrm3_i", *p.enrm3_i},
           {"attribute", *p.attribute}};
}

void from_json(const json& j, KCollisionPrismData& p) {
  p.height = static_cast<float>(j.at("height"));
  p.pos_i = static_cast<int>(j.at("pos_i"));
  p.fnrm_i = static_cast<int>(j.at("fnrm_i"));
  p.enrm1_i = static_cast<int>(j.at("enrm1_i"));
  p.enrm2_i = static_cast<int>(j.at("enrm2_i"));
  p.enrm3_i = static_cast<int>(j.at("enrm3_i"));
  p.attribute = static_cast<int>(j.at("attribute"));
}

// Convert KCollisionData to JSON
void to_json(json& j, const KCollisionData& k) {
  j = json{{"pos_data", k.pos_data},
           {"nrm_data", k.nrm_data},
           {"prism_data", k.prism_data},
           {"block_data", k.block_data},
           {"prism_thickness", k.prism_thickness},
           {"area_min_pos", k.area_min_pos},
           {"area_x_width_mask", k.area_x_width_mask},
           {"area_y_width_mask", k.area_y_width_mask},
           {"area_z_width_mask", k.area_z_width_mask},
           {"block_width_shift", k.block_width_shift},
           {"area_x_blocks_shift", k.area_x_blocks_shift},
           {"area_xy_blocks_shift", k.area_xy_blocks_shift},
           {"sphere_radius", k.sphere_radius},
           {"version", k.version}};
}

// Convert KCollisionData from JSON
void from_json(const json& j, KCollisionData& k) {
  j.at("pos_data").get_to(k.pos_data);
  j.at("nrm_data").get_to(k.nrm_data);
  j.at("prism_data").get_to(k.prism_data);
  j.at("prism_thickness").get_to(k.prism_thickness);
  j.at("area_min_pos").get_to(k.area_min_pos);
  j.at("area_x_width_mask").get_to(k.area_x_width_mask);
  j.at("area_y_width_mask").get_to(k.area_y_width_mask);
  j.at("area_z_width_mask").get_to(k.area_z_width_mask);
  j.at("block_width_shift").get_to(k.block_width_shift);
  j.at("area_x_blocks_shift").get_to(k.area_x_blocks_shift);
  j.at("area_xy_blocks_shift").get_to(k.area_xy_blocks_shift);
  j.at("sphere_radius").get_to(k.sphere_radius);
  j.at("version").get_to(k.version);
}

std::string DumpJSON(const KCollisionData& map) {
  nlohmann::json j = map;
  return j.dump();
}

KCollisionData LoadJSON(std::string_view map) {
  nlohmann::json j = nlohmann::json::parse(map);
  KCollisionData kcl = j.get<KCollisionData>();
  return kcl;
}

} // namespace librii::kcol
