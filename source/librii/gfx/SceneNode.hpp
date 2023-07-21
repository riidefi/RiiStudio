#pragma once

#include <core/common.h>
#include <librii/gfx/MegaState.hpp>
#include <librii/gfx/TextureObj.hpp>
#include <librii/math/aabb.hpp>
#include <rsl/ArrayVector.hpp>
#include <rsl/SmallVector.hpp>

#include <librii/glhelper/UBOBuilder.hpp>

namespace librii::gfx {

enum class PrimitiveType {
  Triangles // GL_TRIANGLES
};

// Convert a PrimitiveType to OpenGL BeginMode
[[nodiscard]] Result<u32> TranslateBeginMode(PrimitiveType t);

enum class DataType {
  Byte,          // GL_BYTE
  UnsignedByte,  // GL_UNSIGNED_BYTE
  Short,         // GL_SHORT
  UnsignedShort, // GL_UNSIGNED_SHORT
  Int,           // GL_INT
  UnsignedInt,   // GL_UNSIGNED_INT
  Float,         // GL_FLOAT
  Double,        // GL_DOUBLE

  S8 = Byte,
  U8 = UnsignedByte,

  S16 = Short,
  U16 = UnsignedShort,

  S32 = Int,
  U32 = UnsignedInt,

  F32 = Float,
  F64 = Double,
};

// Convert a DataType to GL
[[nodiscard]] Result<u32> TranslateDataType(DataType d);

struct SceneNode {
  librii::gfx::MegaState mega_state;
  u32 shader_id;
  u32 vao_id;

  rsl::array_vector<librii::gfx::TextureObj, 8> texture_objects;

  PrimitiveType primitive_type = PrimitiveType::Triangles; // GL_TRIANGLES
  u32 vertex_count;
  DataType vertex_data_type; // DataType::U32 (GL_UNSIGNED_INT)
  void* indices;

  // Expand an AABB with the current bounding box
  //
  // Note: Model-space
  librii::math::AABB bound;

  struct UniformData {
    //! Binding pointer to insert the data at
    u32 binding_point;

    //! Raw data to store there
    //! Note: sizeof(UniformMaterialParams) == 1536
    rsl::small_vector<u8, 2048> raw_data;
  };

  rsl::small_vector<UniformData, 4> uniform_data;

  // TODO: We don't need to set these every draw, just every shader?
  struct UniformMin {
    u32 binding_point;
    u32 min_size;
  };

  rsl::small_vector<UniformMin, 4> uniform_mins;

  // Purely for debugging
  std::string matName;
};

[[nodiscard]] Result<void>
DrawSceneNode(const librii::gfx::SceneNode& node,
              librii::glhelper::DelegatedUBOBuilder& ubo_builder,
              u32 draw_index);

[[nodiscard]] Result<void>
AddSceneNodeToUBO(librii::gfx::SceneNode& node,
                  librii::glhelper::DelegatedUBOBuilder& ubo_builder);

} // namespace librii::gfx
