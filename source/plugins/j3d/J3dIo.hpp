#pragma once

#include "Scene.hpp"
#include <core/kpi/Plugins.hpp>

#include <rsl/Ranges.hpp>

namespace riistudio::j3d {

struct J3dModel {
  ScalingRule scalingRule = ScalingRule::Basic;
  bool isBDL = false;
  riistudio::j3d::Bufs vertexData;
  std::vector<libcube::DrawMatrix> drawMatrices;
  std::vector<librii::j3d::MaterialData> materials;
  std::vector<librii::j3d::JointData> joints;
  std::vector<u32> jointIds;
  std::vector<librii::j3d::ShapeData> shapes;
  std::vector<librii::j3d::TextureData> textures;

  static Result<J3dModel> read(oishii::BinaryReader& reader,
                               kpi::LightIOTransaction& tx);
  Result<void> write(oishii::Writer& writer);
};

inline void readJ3dMdl(J3dModel& m, const riistudio::j3d::Model& editor,
                       riistudio::j3d::Collection& c) {
  m.scalingRule = editor.info.mScalingRule;
  m.isBDL = editor.isBDL;
  m.vertexData = editor.mBufs;
  m.drawMatrices = editor.mDrawMatrices;
  for (auto& mat : editor.getMaterials()) {
    m.materials.emplace_back(mat);
  }
  for (auto& b : editor.getBones()) {
    m.joints.emplace_back(b);
    m.jointIds.emplace_back(b.id);
  }
  for (auto& mesh : editor.getMeshes()) {
    m.shapes.emplace_back(mesh);
  }
  for (auto& tex : c.getTextures()) {
    m.textures.emplace_back(tex);
  }
}
inline void toEditorMdl(riistudio::j3d::Collection& s, const J3dModel& m) {
  riistudio::j3d::Model& tmp = s.getModels().add();
  tmp.info.mScalingRule = m.scalingRule;
  tmp.isBDL = m.isBDL;
  tmp.mBufs = m.vertexData;
  tmp.mDrawMatrices = m.drawMatrices;
  for (auto& ma : m.materials)
    static_cast<librii::j3d::MaterialData&>(tmp.getMaterials().add()) = ma;
  size_t i = 0;
  for (auto& b : m.joints) {
    auto& added = tmp.getBones().add();
    static_cast<librii::j3d::JointData&>(added) = b;
    added.id = m.jointIds[i++];
  }
  for (auto& mx : m.shapes)
    static_cast<librii::j3d::ShapeData&>(tmp.getMeshes().add()) = mx;
  for (auto& t : m.textures)
    static_cast<librii::j3d::TextureData&>(s.getTextures().add()) = t;
}

inline Result<void> WriteBMD(riistudio::j3d::Collection& collection,
                             oishii::Writer& writer) {
  J3dModel tmp;
  readJ3dMdl(tmp, collection.getModels()[0], collection);
  return tmp.write(writer);
}
inline Result<void> ReadBMD(riistudio::j3d::Collection& collection,
                            oishii::BinaryReader& reader,
                            kpi::LightIOTransaction& transaction) {
  auto tmp = TRY(J3dModel::read(reader, transaction));
  toEditorMdl(collection, tmp);
  collection.onRelocate();
  collection.getModels()[0].onRelocate();
  return {};
}

} // namespace riistudio::j3d
