#pragma once

#include <core/common.h>
#include <span>
#include <string>
#include <vector>

namespace librii::lettuce {

//! Defines a partioning of a LEX file holding raw sections
struct LEXParts {
  struct Section {
    u32 magic;
    std::vector<u8> data;
  };
  std::vector<Section> sections;
};

std::expected<LEXParts, std::string> ReadLEXParts(std::span<const u8> data);
std::expected<std::vector<u8>, std::string>
WriteLEXParts(const LEXParts& parts);

struct LEXIdentifier {
  enum {
    Invalidated = 0x2d2d2d2d, // '----'
    FEAT = 0x46454154,
    SET1 = 0x53455431,
    CANN = 0x43414e4e,
    HIPT = 0x48495054,
    TEST = 0x54455354,
  };
};

} // namespace librii::lettuce
