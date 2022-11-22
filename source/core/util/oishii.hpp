#pragma once

#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <optional>
#include <plate/Platform.hpp>
#include <rsl/Expected.hpp>

inline std::optional<oishii::DataProvider>
OishiiReadFile(std::string_view path) {
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
inline rsl::expected<std::vector<u8>, std::string> ReadFile(std::string_view path) {
  auto buf = OishiiReadFile(path);
  if (!buf) {
    return "Failed to read file at \"" + std::string(path) + "\"";
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

inline void OishiiFlushWriter(oishii::Writer& writer, std::string_view path) {
  // On the Web target, this will download a file to the user's browser
  plate::Platform::writeFile({writer.getDataBlockStart(), writer.getBufSize()},
                             path);
}
