#include "Common.hpp"
#include <core/common.h>
#include <core/kpi/Plugins.hpp>
#include <librii/g3d/io/BoneIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/MatIO.hpp>
#include <librii/g3d/io/ModelIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>
#include <variant>

namespace riistudio::g3d {

void ApplyBytecodeToDrawMatrices(
    riistudio::g3d::Model& mdl,
    librii::g3d::ByteCodeLists::NodeDescendence* desc, int& base_matrix_id) {
  const auto& bone = mdl.getBones()[desc->boneId];
  const auto matrixId = bone.matrixId;

  // Hack: Base matrix is first copied to end, first instruction
  if (base_matrix_id == -1) {
    base_matrix_id = matrixId;
  } else {
    auto& drws = mdl.mDrawMatrices;
    if (drws.size() <= matrixId) {
      drws.resize(matrixId + 1);
    }
    // TODO: Start at 100?
    drws[matrixId].mWeights.emplace_back(desc->boneId, 1.0f);
  }
  // printf("BoneIdx: %u (MatrixIdx: %u), ParentMtxIdx: %u\n",
  // (u32)boneIdx,
  //        (u32)matrixId, (u32)parentMtxIdx);
  // printf("\"Matrix %u\" -> \"Matrix %u\" [label=\"parent\"];\n",
  //        (u32)matrixId, (u32)parentMtxIdx);
}

void CheckMaterialXluFlag(riistudio::g3d::Material& mat,
                          librii::g3d::ByteCodeMethod& method,
                          riistudio::g3d::Bone::Display& disp,
                          riistudio::g3d::Polygon& poly,
                          kpi::LightIOTransaction& transaction,
                          const std::string& transaction_path) {
  const bool xlu_mat = mat.flag & 0x80000000;
  if ((method.name == "DrawOpa" && xlu_mat) ||
      (method.name == "DrawXlu" && !xlu_mat)) {
    char warn_msg[1024]{};
    const auto mat_name = mat.name;
    snprintf(warn_msg, sizeof(warn_msg),
             "Material %u \"%s\" is rendered in the %s pass (with mesh "
             "%u \"%s\"), but is marked as %s.",
             disp.matId, mat_name.c_str(), xlu_mat ? "Opaue" : "Translucent",
             disp.polyId, poly.getName().c_str(),
             !xlu_mat ? "Opaque" : "Translucent");
    transaction.callback(kpi::IOMessageClass::Warning,
                         transaction_path + "materials/" + mat_name, warn_msg);
  }
}

void ApplyDrawCallToModel(librii::g3d::ByteCodeLists::Draw* draw,
                          riistudio::g3d::Model& mdl,
                          kpi::LightIOTransaction& transaction,
                          const std::string& transaction_path,
                          librii::g3d::ByteCodeMethod& method) {
  Bone::Display disp{
      .matId = static_cast<u32>(draw->matId),
      .polyId = static_cast<u32>(draw->polyId),
      .prio = draw->prio,
  };
  auto boneIdx = draw->boneId;
  if (boneIdx > mdl.getBones().size()) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "Invalid bone index in render command");
    boneIdx = 0;
    transaction.state = kpi::TransactionState::Failure;
  }

  if (disp.matId > mdl.getMeshes().size()) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "Invalid material index in render command");
    disp.matId = 0;
    transaction.state = kpi::TransactionState::Failure;
  }

  if (disp.polyId > mdl.getMeshes().size()) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "Invalid mesh index in render command");
    disp.polyId = 0;
    transaction.state = kpi::TransactionState::Failure;
  }

  mdl.getBones()[boneIdx].addDisplay(disp);

  // While with this setup, materials could be XLU and OPA, in
  // practice, they're not.
  //
  // Warn the user if a material is flagged as OPA/XLU but doesn't exist
  // in the corresponding draw list.
  auto& mat = mdl.getMaterials()[disp.matId];
  auto& poly = mdl.getMeshes()[disp.polyId];
  CheckMaterialXluFlag(mat, method, disp, poly, transaction, transaction_path);
  // And ultimately reset the flag to its proper value.
  mat.xlu = method.name == "DrawXlu";
}

void processModel(librii::g3d::BinaryModel& binary_model,
                  kpi::LightIOTransaction& transaction,
                  const std::string& transaction_path,
                  riistudio::g3d::Model& mdl) {
  using namespace librii::g3d;
  if (transaction.state == kpi::TransactionState::Failure) {
    return;
  }
  // Assign info
  static_cast<librii::g3d::G3DModelDataData&>(mdl) = binary_model.info;

  mdl.getBones().resize(binary_model.bones.size());
  for (size_t i = 0; i < binary_model.bones.size(); ++i) {
    static_cast<librii::g3d::BoneData&>(mdl.getBones()[i]) =
        binary_model.bones[i];
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

  // TODO: Fura

  for (auto& mat : binary_model.materials) {
    static_cast<librii::g3d::G3dMaterialData&>(mdl.getMaterials().add()) = mat;
  }
  for (auto& mesh : binary_model.meshes) {
    static_cast<librii::g3d::PolygonData&>(mdl.getMeshes().add()) = mesh;
  }

  // Process bytecode: Apply to materials/bones/draw matrices

  for (auto& method : binary_model.bytecodes) {
    int base_matrix_id = -1; // Modified by ApplyBytecodeToDrawMatrices

    // printf("digraph {\n");
    for (auto& command : method.commands) {
      if (auto* draw = std::get_if<ByteCodeLists::Draw>(&command);
          draw != nullptr) {
        ApplyDrawCallToModel(draw, mdl, transaction, transaction_path, method);
      } else if (auto* desc =
                     std::get_if<ByteCodeLists::NodeDescendence>(&command);
                 desc != nullptr) {
        ApplyBytecodeToDrawMatrices(mdl, desc, base_matrix_id);
      }
      // TODO: Other bytecodes
    }
    // printf("}\n");
  }
}


} // namespace riistudio::g3d
