#pragma once

#include "Scene.hpp"
#include <LibBadUIFramework/Plugins.hpp>

#include <rsl/Ranges.hpp>

#include <librii/j3d/J3dIo.hpp>

namespace riistudio::j3d {

static inline void readJ3dMdl(librii::j3d::J3dModel& m,
                              const riistudio::j3d::Model& editor,
                              riistudio::j3d::Collection& c) {
  m.scalingRule = editor.mScalingRule;
  m.isBDL = editor.isBDL;
  m.vertexData = editor.mBufs;
  m.drawMatrices = editor.mDrawMatrices;
  for (auto& mat : editor.getMaterials()) {
    m.materials.emplace_back(mat);
  }
  for (auto& b : editor.getBones()) {
    m.joints.emplace_back(b.compile());
  }
  for (auto& mesh : editor.getMeshes()) {
    m.shapes.emplace_back(mesh);
  }
  for (auto& tex : c.getTextures()) {
    m.textures.emplace_back(tex);
  }
}
static inline void toEditorMdl(riistudio::j3d::Collection& s,
                               const librii::j3d::J3dModel& m) {
  riistudio::j3d::Model& tmp = s.getModels().add();
  tmp.mScalingRule = m.scalingRule;
  tmp.isBDL = m.isBDL;
  tmp.mBufs = m.vertexData;
  tmp.mDrawMatrices = m.drawMatrices;
  for (auto& ma : m.materials)
    static_cast<librii::j3d::MaterialData&>(tmp.getMaterials().add()) = ma;
  for (auto& b : m.joints) {
    auto& added = tmp.getBones().add();
    added.decompile(b);
  }
  for (auto& mx : m.shapes)
    static_cast<librii::j3d::ShapeData&>(tmp.getMeshes().add()) = mx;
  for (auto& t : m.textures)
    static_cast<librii::j3d::TextureData&>(s.getTextures().add()) = t;
}

[[nodiscard]] static inline Result<void>
WriteBMD(riistudio::j3d::Collection& collection, oishii::Writer& writer,
         bool linkmap = true) {
  librii::j3d::J3dModel tmp;
  readJ3dMdl(tmp, collection.getModels()[0], collection);
  return tmp.write(writer, linkmap);
}
[[nodiscard]] static inline Result<void>
ReadBMD(riistudio::j3d::Collection& collection, oishii::BinaryReader& reader,
        kpi::LightIOTransaction& transaction) {
  auto tmp = TRY(librii::j3d::J3dModel::read(reader, transaction));
  toEditorMdl(collection, tmp);
  collection.onRelocate();
  collection.getModels()[0].onRelocate();
  return {};
}
[[nodiscard]] static inline Result<void>
ReadBMD(riistudio::j3d::Collection& collection, oishii::BinaryReader& reader) {
  kpi::LightIOTransaction trans;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    rsl::error(msg);
  };
  auto tmp = TRY(librii::j3d::J3dModel::read(reader, trans));
  toEditorMdl(collection, tmp);
  collection.onRelocate();
  collection.getModels()[0].onRelocate();
  return {};
}
[[nodiscard]] static inline Result<Collection> ReadBMD(std::string path) {
  Collection c;
  auto reader = TRY(oishii::BinaryReader::FromFilePath(path, std::endian::big));
  TRY(ReadBMD(c, reader));
  return c;
}

} // namespace riistudio::j3d
