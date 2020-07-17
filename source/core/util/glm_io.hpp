#pragma once

#include <vendor/glm/vec3.hpp>
#include <vendor/glm/vec2.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

inline void read(oishii::BinaryReader& reader, glm::vec3& out) {
  out.x = reader.read<f32>();
  out.y = reader.read<f32>();
  out.z = reader.read<f32>();
}

inline void read(oishii::BinaryReader& reader, glm::vec2& out) {
  out.x = reader.read<f32>();
  out.y = reader.read<f32>();
}
inline void operator<<(glm::vec3& out, oishii::BinaryReader& reader) {
  out.x = reader.read<f32>();
  out.y = reader.read<f32>();
  out.z = reader.read<f32>();
}
inline void operator<<(glm::vec2& out, oishii::BinaryReader& reader) {
  out.x = reader.read<f32>();
  out.y = reader.read<f32>();
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
