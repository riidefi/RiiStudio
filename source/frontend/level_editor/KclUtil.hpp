#pragma once

#include <core/common.h>
#include <rsl/ColorUtil.hpp>
#include <string>

enum class Form {
  Standard,
  Indexed,

  UNKNOWN
};

struct KclType {
  u32 id;
  Form form;
  std::string name;
};

const KclType& GetKCLType(u32 attr);

inline glm::vec3
GetKCLColor(u32 attr, glm::vec3 base_color = rsl::color_from_hex(0xf4a26188)) {
  const auto attr_fraction = static_cast<float>(attr & 31) / 31.0f;

  return rsl::hue_translate(base_color, attr_fraction * 360.0f + 60.0f);
}
