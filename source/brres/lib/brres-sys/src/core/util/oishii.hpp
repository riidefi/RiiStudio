#pragma once

#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <optional>
#include <plate/Platform.hpp>
#include <rsl/Expected.hpp>

inline std::optional<std::vector<u8>>
OishiiReadFile2(std::string_view path) {
  std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
  if (!file)
    return std::nullopt;

  std::vector<u8> vec(file.tellg());
  file.seekg(0, std::ios::beg);

  if (!file.read(reinterpret_cast<char*>(vec.data()), vec.size())) {
    return std::nullopt;
  }

  return vec;
}
inline std::expected<std::vector<u8>, std::string>
ReadFile(std::string_view path) {
  auto buf = OishiiReadFile2(path);
  if (!buf) {
    return RSL_UNEXPECTED("Failed to read file at \"" + std::string(path) +
                           "\"");
  }
  return *buf;
}

inline std::vector<u8> OishiiReadFile(std::string display_path,
                                           const u8* data, size_t len) {
  std::vector<u8> vec(data, data + len);

  return vec;
}
