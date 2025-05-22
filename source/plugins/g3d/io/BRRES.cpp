// Hacky workarounds for enabling designated initializers
#define ARCHIVE_DEF
#define MODEL_DEF

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/Plugins.hpp>
#include <core/common.h>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

#include <librii/g3d/io/DictWriteIO.hpp>
#include <plugins/g3d/collection.hpp>

#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/ArchiveIO.hpp>

#include <rsl/Ranges.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace riistudio::g3d {

static Result<void> processModel(librii::g3d::Model& binary_model,
                                 const std::string& transaction_path,
                                 riistudio::g3d::Model& mdl) {
  mdl.mName = binary_model.name;

  {
    const auto& info = binary_model.info;
    mdl.mScalingRule = info.scalingRule;
    mdl.mTexMtxMode = info.texMtxMode;
    mdl.sourceLocation = info.sourceLocation;
    mdl.mEvpMtxMode = info.evpMtxMode;
    mdl.aabb.min = info.min;
    mdl.aabb.max = info.max;
  }

  mdl.getBones().resize(binary_model.bones.size());
  for (size_t i = 0; i < binary_model.bones.size(); ++i) {
    mdl.getBones()[i].decompile(binary_model.bones[i]);
  }

  for (auto& pos : binary_model.positions) {
    static_cast<librii::g3d::PositionBuffer&>(mdl.getBuf_Pos().add()) = pos;
  }
  for (auto& norm : binary_model.normals) {
    static_cast<librii::g3d::NormalBuffer&>(mdl.getBuf_Nrm().add()) = norm;
  }
  for (auto& color : binary_model.colors) {
    static_cast<librii::g3d::ColorBuffer&>(mdl.getBuf_Clr().add()) = color;
  }
  for (auto& texcoord : binary_model.texcoords) {
    static_cast<librii::g3d::TextureCoordinateBuffer&>(mdl.getBuf_Uv().add()) =
        texcoord;
  }
  // TODO: Fur
  for (auto& mat : binary_model.materials) {
    static_cast<librii::g3d::G3dMaterialData&>(mdl.getMaterials().add()) = mat;
  }
  for (auto& mesh : binary_model.meshes) {
    static_cast<librii::g3d::PolygonData&>(mdl.getMeshes().add()) = mesh;
  }

  mdl.mDrawMatrices.resize(0);
  for (auto& mtx : binary_model.matrices) {
    libcube::DrawMatrix drw;
    drw.mWeights.resize(0);
    for (auto& p : mtx.mWeights) {
      drw.mWeights.emplace_back(p.boneId, p.weight);
    }
    mdl.mDrawMatrices.emplace_back(drw);
  }

  return {};
}

Result<void> ReadBRRES(Collection& collection, librii::g3d::Archive& archive,
                       std::string path) {
  collection.path = path;
  for (auto& mdl : archive.models) {
    auto& editor_mdl = collection.getModels().add();
    auto ok = processModel(mdl, "MDL0 " + mdl.name, editor_mdl);
    if (!ok) {
      return std::unexpected(
          std::format("Could not read MDL0 {}: {}", mdl.name, ok.error()));
    }
  }
  for (auto& tex : archive.textures) {
    static_cast<librii::g3d::TextureData&>(collection.getTextures().add()) =
        tex;
  }
  for (auto& srt : archive.srts) {
    static_cast<librii::g3d::SrtAnimationArchive&>(
        collection.getAnim_Srts().add()) = srt;
  }
  for (auto& clr : archive.clrs) {
    static_cast<librii::g3d::ClrAnim&>(collection.getAnim_Clrs().add()) = clr;
  }
  collection.chrs = archive.chrs;
  collection.pats = archive.pats;
  collection.viss = archive.viss;
  return {};
}

Result<void> ReadBRRES(Collection& collection, oishii::BinaryReader& reader,
                       kpi::LightIOTransaction& transaction) {
  auto archive = TRY(librii::g3d::Archive::read(reader, transaction));
  return ReadBRRES(collection, archive, reader.getFile());
}

librii::g3d::Model toBinaryModel(const Model& mdl) {
  librii::g3d::Model intermediate{
      .name = mdl.mName,
      .positions =
          mdl.getBuf_Pos() | rsl::ToList<librii::g3d::PositionBuffer>(),
      .normals = mdl.getBuf_Nrm() | rsl::ToList<librii::g3d::NormalBuffer>(),
      .colors = mdl.getBuf_Clr() | rsl::ToList<librii::g3d::ColorBuffer>(),
      .texcoords =
          mdl.getBuf_Uv() | rsl::ToList<librii::g3d::TextureCoordinateBuffer>(),
      .materials = mdl.getMaterials() //
                   | rsl::ToList<librii::g3d::G3dMaterialData>(),
      .meshes = mdl.getMeshes() | rsl::ToList<librii::g3d::PolygonData>(),
  };
  for (auto& b : mdl.getBones()) {
    intermediate.bones.push_back(b.compile());
  }
  intermediate.matrices.resize(0);
  for (auto& mtx : mdl.mDrawMatrices) {
    librii::g3d::DrawMatrix drw;
    drw.mWeights.resize(0);
    for (auto& p : mtx.mWeights) {
      drw.mWeights.emplace_back(p.boneId, p.weight);
    }
    intermediate.matrices.emplace_back(drw);
  }
  intermediate.info = librii::g3d::ModelInfo{
      .scalingRule = mdl.mScalingRule,
      .texMtxMode = mdl.mTexMtxMode,
      .sourceLocation = mdl.sourceLocation,
      .evpMtxMode = mdl.mEvpMtxMode,
      .min = mdl.aabb.min,
      .max = mdl.aabb.max,
  };
  return intermediate;
}

librii::g3d::Archive Collection::toLibRii() const {
  librii::g3d::Archive arc{
      .textures = getTextures() | rsl::ToList<librii::g3d::TextureData>(),
      .chrs = chrs,
      .clrs = getAnim_Clrs() | rsl::ToList<librii::g3d::ClrAnim>(),
      .pats = pats,
      .srts = getAnim_Srts() | rsl::ToList<librii::g3d::SrtAnimationArchive>(),
      .viss = viss,
  };
  for (auto& mdl : getModels()) {
    arc.models.push_back(toBinaryModel(mdl));
  }
  return arc;
}

Result<void> WriteBRRES(Collection& scn, oishii::Writer& writer) {
  auto arc = scn.toLibRii();
  return arc.write(writer);
}

} // namespace riistudio::g3d
