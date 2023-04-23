#include "LettuceLEX.hpp"

#include <rsl/SafeReader.hpp>
#include <rsl/SimpleReader.hpp>

namespace librii::lettuce {

constexpr u32 LEX_MAGIC = 0x4C452D58; // 'LE-X'
constexpr u16 LEX_REV_MAJOR = 1;
constexpr u16 LEX_REV_MINOR = 0;

struct LEXHeader {
  rsl::bu32 magic{LEX_MAGIC};
  rsl::bu16 rev_major{LEX_REV_MAJOR};
  rsl::bu16 rev_minor{LEX_REV_MINOR};
  rsl::bu32 filesize{0};
  rsl::bu32 first_section{0x10};
};

std::expected<LEXParts, std::string> ReadLEXParts(std::span<const u8> data) {
  if (data.size() < sizeof(LEXHeader)) {
    return std::unexpected("Invalid header length");
  }
  LEXHeader header{};
  memcpy(&header, data.data(), sizeof(header));
  if (header.magic != LEX_MAGIC) {
    return std::unexpected("Invalid file magic");
  }
  if (header.rev_major != LEX_REV_MAJOR) {
    return std::unexpected("Invalid version");
  }
  if ((header.filesize & 3) || (header.first_section & 3)) {
    return std::unexpected("Misaligned data");
  }
  if (header.filesize > data.size() ||
      header.first_section < sizeof(LEXHeader) ||
      header.first_section + 4 > header.filesize) {
    return std::unexpected("Truncated file");
  }
  auto chain =
      data.subspan(sizeof(LEXHeader), header.filesize - sizeof(LEXHeader));

  LEXParts parts;
  for (u32 it = 0; it < chain.size();) {
    u32 magic = rsl::load<u32>(chain, it);
    if (magic == 0) {
      break;
    }
    if (chain.size() - it < 8) {
      return std::unexpected(std::format("Truncated section {:x}\n", magic));
    }
    u32 size = rsl::load<u32>(chain, it);
    if ((size & 3) || it + 8 + size > chain.size()) {
      return std::unexpected("Invalid data size");
    }
    LEXParts::Section section{.magic = magic};
    section.data.insert(section.data.begin(), chain.begin() + it + 8,
                        chain.begin() + it + 8 + size);
    parts.sections.push_back(std::move(section));
    it += 8 + size;
  }
  return parts;
}

std::expected<std::vector<u8>, std::string>
WriteLEXParts(const LEXParts& parts) {
  LEXHeader header{};
  std::vector<u8> result(sizeof(header));

  for (auto& section : parts.sections) {
    result.resize(result.size() + 8);
    rsl::store(section.magic, result, result.size() - 8);
    rsl::store(TRY(rsl::checked_cast<u32>(section.data.size())), result,
               result.size() - 4);
    result.insert(result.end(), section.data.begin(), section.data.end());
  }

  header.filesize = TRY(rsl::checked_cast<u32>(result.size()));
  memcpy(result.data(), &header, sizeof(header));
  return result;
}

} // namespace librii::lettuce
