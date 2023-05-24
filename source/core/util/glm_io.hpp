#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vendor/glm/vec2.hpp>
#include <vendor/glm/vec3.hpp>

#include <core/common.h>

[[nodiscard]] inline Result<void> read(oishii::BinaryReader& reader,
                                       glm::vec3& out) {
  out.x = TRY(reader.tryRead<f32>());
  out.y = TRY(reader.tryRead<f32>());
  out.z = TRY(reader.tryRead<f32>());
  return {};
}

[[nodiscard]] inline Result<void> read(oishii::BinaryReader& reader,
                                       glm::vec2& out) {
  out.x = TRY(reader.tryRead<f32>());
  out.y = TRY(reader.tryRead<f32>());
  return {};
}
[[nodiscard]] inline Result<void> operator<<(glm::vec3& out,
                                             oishii::BinaryReader& reader) {
  return read(reader, out);
}
[[nodiscard]] inline Result<void> operator<<(glm::vec2& out,
                                             oishii::BinaryReader& reader) {
  return read(reader, out);
}
inline void operator>>(const glm::vec3& vec, oishii::Writer& writer) {
  writer.write(vec.x);
  writer.write(vec.y);
  writer.write(vec.z);
}
inline void operator>>(const glm::vec2& vec, oishii::Writer& writer) {
  writer.write(vec.x);
  writer.write(vec.y);
}
