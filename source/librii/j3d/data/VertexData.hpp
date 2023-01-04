#pragma once

#include <librii/gx/Vertex.hpp>

namespace librii::j3d {

enum class ScalingRule { Basic, XSI, Maya };
using librii::gx::VBufferKind;
using librii::gx::VertexBuffer;
using librii::gx::VQuantization;

struct Bufs {
  static inline VertexBuffer<gx::Color, VBufferKind::color> C = {
      gx::VQuantization{
          gx::VertexComponentCount(gx::VertexComponentCount::Color::rgba),
          gx::VertexBufferType(gx::VertexBufferType::Color::FORMAT_32B_8888), 0,
          0, 4}};
  static inline VertexBuffer<glm::vec2, VBufferKind::textureCoordinate> U = {
      gx::VQuantization{
          gx::VertexComponentCount(
              gx::VertexComponentCount::TextureCoordinate::st),
          gx::VertexBufferType(gx::VertexBufferType::Generic::f32), 0, 0, 8}};

  // FIXME: Good default values
  VertexBuffer<glm::vec3, VBufferKind::position> pos{gx::VQuantization{
      gx::VertexComponentCount(gx::VertexComponentCount::Position::xyz),
      gx::VertexBufferType(gx::VertexBufferType::Generic::f32), 0, 0, 12}};
  VertexBuffer<glm::vec3, VBufferKind::normal> norm{gx::VQuantization{
      gx::VertexComponentCount(gx::VertexComponentCount::Normal::xyz),
      gx::VertexBufferType(gx::VertexBufferType::Generic::f32), 0, 0, 12}};
  std::array<VertexBuffer<gx::Color, VBufferKind::color>, 2> color{C, C};
  std::array<VertexBuffer<glm::vec2, VBufferKind::textureCoordinate>, 8> uv{
      U, U, U, U, U, U, U, U};
};

} // namespace librii::j3d
