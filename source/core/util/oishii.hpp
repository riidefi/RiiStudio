#pragma once

#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <optional>
#include <plate/Platform.hpp>
#include <rsl/Expected.hpp>

inline std::optional<oishii::DataProvider>
OishiiReadFile2(std::string_view path) {
  std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
  if (!file)
    return std::nullopt;

  std::vector<u8> vec(file.tellg());
  file.seekg(0, std::ios::beg);

  if (!file.read(reinterpret_cast<char*>(vec.data()), vec.size())) {
    return std::nullopt;
  }

  return oishii::DataProvider{std::move(vec), path};
}
inline std::expected<std::vector<u8>, std::string>
ReadFile(std::string_view path) {
  auto buf = OishiiReadFile2(path);
  if (!buf) {
    return std::unexpected("Failed to read file at \"" + std::string(path) +
                           "\"");
  }
  // Cannot directly access vector
  auto slice = buf->slice();
  std::vector<u8> bruh(slice.begin(), slice.end());
  return bruh;
}

inline oishii::DataProvider OishiiReadFile(std::string display_path,
                                           const u8* data, size_t len) {
  std::vector<u8> vec(data, data + len);

  return oishii::DataProvider{std::move(vec), display_path};
}
