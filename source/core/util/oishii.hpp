#pragma once

#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <optional>
#include <plate/Platform.hpp>

inline std::optional<oishii::DataProvider> OishiiReadFile(std::string path) {
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

inline void OishiiFlushWriter(oishii::Writer& writer, std::string_view path) {
  // On the Web target, this will download a file to the user's browser
  plate::Platform::writeFile({writer.getDataBlockStart(), writer.getBufSize()},
                             path);
}
