#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vendor/glm/vec2.hpp>
#include <vendor/glm/vec3.hpp>

inline void read(oishii::BinaryReader& reader, glm::vec3& out) {
  if (reader.tell() + 12 <= reader.endpos()) {
    out.x = reader.tryRead<f32>().value();
    out.y = reader.tryRead<f32>().value();
    out.z = reader.tryRead<f32>().value();
  } else {
    out = {};
  }
}

inline void read(oishii::BinaryReader& reader, glm::vec2& out) {
  if (reader.tell() + 8 <= reader.endpos()) {
    out.x = reader.tryRead<f32>().value();
    out.y = reader.tryRead<f32>().value();
  } else {
    out = {};
  }
}
inline void operator<<(glm::vec3& out, oishii::BinaryReader& reader) {
  read(reader, out);
}
inline void operator<<(glm::vec2& out, oishii::BinaryReader& reader) {
  read(reader, out);
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
