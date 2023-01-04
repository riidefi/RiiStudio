#pragma once

#include <librii/j3d/data/JointData.hpp>
#include <librii/j3d/data/MaterialData.hpp>
#include <librii/j3d/data/ShapeData.hpp>
#include <librii/j3d/data/TextureData.hpp>
#include <librii/j3d/data/VertexData.hpp>

namespace librii::j3d {

struct J3dModel {
  ScalingRule scalingRule = ScalingRule::Basic;
  bool isBDL = false;
  Bufs vertexData;
  std::vector<libcube::DrawMatrix> drawMatrices;
  std::vector<librii::j3d::MaterialData> materials;
  std::vector<librii::j3d::JointData> joints;
  std::vector<u32> jointIds; // TODO: Shouldn't need this
  std::vector<librii::j3d::ShapeData> shapes;
  std::vector<librii::j3d::TextureData> textures;

  static Result<J3dModel> read(oishii::BinaryReader& reader,
                               kpi::LightIOTransaction& tx);
  Result<void> write(oishii::Writer& writer);
};

} // namespace librii::j3d
