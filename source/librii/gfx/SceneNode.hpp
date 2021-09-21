#pragma once

#include <core/common.h>
#include <librii/gfx/MegaState.hpp>
#include <librii/gfx/TextureObj.hpp>
#include <librii/math/aabb.hpp>
#include <llvm/ADT/SmallVector.h>
#include <rsl/ArrayVector.hpp>

#include <librii/glhelper/UBOBuilder.hpp>

namespace librii::gfx {

struct SceneNode {
  librii::gfx::MegaState mega_state;
  u32 shader_id;
  u32 vao_id;

  rsl::array_vector<librii::gfx::TextureObj, 8> texture_objects;

  u32 glBeginMode; // GL_TRIANGLES
  u32 vertex_count;
  u32 glVertexDataType; // GL_UNSIGNED_INT
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
    llvm::SmallVector<u8, 2048> raw_data;
  };

  llvm::SmallVector<UniformData, 4> uniform_data;

  // TODO: We don't need to set these every draw, just every shader?
  struct UniformMin {
    u32 binding_point;
    u32 min_size;
  };

  llvm::SmallVector<UniformMin, 4> uniform_mins;
};

void DrawSceneNode(const librii::gfx::SceneNode& node,
                   librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                   u32 draw_index);

void AddSceneNodeToUBO(librii::gfx::SceneNode& node,
                       librii::glhelper::DelegatedUBOBuilder& ubo_builder);

} // namespace librii::gfx
