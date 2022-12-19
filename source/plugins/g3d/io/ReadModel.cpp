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

struct IOContext {
  std::string path;
  kpi::LightIOTransaction& transaction;

  auto sublet(const std::string& dir) {
    return IOContext(path + "/" + dir, transaction);
  }

  void callback(kpi::IOMessageClass mclass, std::string_view message) {
    transaction.callback(mclass, path, message);
  }

  void inform(std::string_view message) {
    callback(kpi::IOMessageClass::Information, message);
  }
  void warn(std::string_view message) {
    callback(kpi::IOMessageClass::Warning, message);
  }
  void error(std::string_view message) {
    callback(kpi::IOMessageClass::Error, message);
  }
  void request(bool cond, std::string_view message) {
    if (!cond)
      warn(message);
  }
  void require(bool cond, std::string_view message) {
    if (!cond)
      error(message);
  }

  template <typename... T>
  void request(bool cond, std::string_view f_, T&&... args) {
    // TODO: Use basic_format_string
    auto msg = std::vformat(f_, std::make_format_args(args...));
    request(cond, msg);
  }

  IOContext(std::string&& p, kpi::LightIOTransaction& t)
      : path(std::move(p)), transaction(t) {}
  IOContext(kpi::LightIOTransaction& t) : transaction(t) {}
};

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
  IOContext ctx("MDL0 " + binary_model.name, transaction);

  mdl.mName = binary_model.name;
  // Assign info
  {
    const auto& info = binary_model.info;
    mdl.mScalingRule = info.scalingRule;
    mdl.mTexMtxMode = info.texMtxMode;
    // Validate numVerts, numTris
    {
      auto [computedNumVerts, computedNumTris] =
          librii::gx::computeVertTriCounts(binary_model.meshes);
      ctx.request(
          computedNumVerts == info.numVerts,
          "Model header specifies {} vertices, but the file only has {}.",
          info.numVerts, computedNumVerts);
      ctx.request(
          computedNumTris == info.numTris,
          "Model header specifies {} triangles, but the file only has {}.",
          info.numTris, computedNumTris);
    }
    mdl.sourceLocation = info.sourceLocation;
    {
      auto displayMatrices =
          librii::gx::computeDisplayMatricesSubset(binary_model.meshes);
      ctx.request(info.numViewMtx == displayMatrices.size(),
                  "Model header specifies {} display matrices, but the mesh "
                  "data only references {} display matrices.",
                  info.numViewMtx, displayMatrices.size());
    }
    {
      auto needsNormalMtx =
          std::any_of(binary_model.meshes.begin(), binary_model.meshes.end(),
                      [](auto& m) { return m.needsNormalMtx(); });
      ctx.request(
          info.normalMtxArray == needsNormalMtx,
          needsNormalMtx
              ? "Model header unecessarily burdens the runtime library "
                "with bone-normal-matrix computation"
              : "Model header does not tell the runtime library to maintain "
                "bone normal matrix arrays, although some meshes need it");
    }
    {
      auto needsTexMtx =
          std::any_of(binary_model.meshes.begin(), binary_model.meshes.end(),
                      [](auto& m) { return m.needsTextureMtx(); });
      ctx.request(
          info.texMtxArray == needsTexMtx,
          needsTexMtx
              ? "Model header unecessarily burdens the runtime library "
                "with bone-texture-matrix computation"
              : "Model header does not tell the runtime library to maintain "
                "bone texture matrix arrays, although some meshes need it");
    }
    ctx.request(!info.boundVolume,
                "Model specifies bounding data should be used");
    mdl.mEvpMtxMode = info.evpMtxMode;
    mdl.aabb.min = info.min;
    mdl.aabb.max = info.max;
    // Validate mtxToBoneLUT
    {
      const auto& lut = info.mtxToBoneLUT.mtxIdToBoneId;
      for (size_t i = 0; i < binary_model.bones.size(); ++i) {
        const auto& bone = binary_model.bones[i];
        if (bone.matrixId > lut.size()) {
          ctx.error(
              std::format("Bone {} specifies a matrix ID of {}, but the matrix "
                          "LUT only specifies {} matrices total.",
                          bone.mName, bone.matrixId, lut.size()));
          continue;
        }
        ctx.request(lut[bone.matrixId] == i,
                    "Bone {} (#{}) declares ownership of Matrix{}. However, "
                    "Matrix{} does not register this bone as its owner. "
                    "Rather, it specifies an owner ID of {}.",
                    bone.mName, i, bone.matrixId, bone.matrixId,
                    lut[bone.matrixId]);
      }
    }
  }

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
    for (auto& command : method.commands) {
      if (auto* draw = std::get_if<ByteCodeLists::Draw>(&command)) {
        ApplyDrawCallToModel(draw, mdl, transaction, transaction_path, method);
      } else if (auto* desc =
                     std::get_if<ByteCodeLists::NodeDescendence>(&command)) {
        auto& bone = mdl.getBones()[desc->boneId];
        const auto matrixId = bone.matrixId;

        auto parent_id =
            binary_model.info.mtxToBoneLUT.mtxIdToBoneId[desc->parentMtxId];
        if (bone.mParent != -1 && parent_id >= 0) {
          bone.mParent = parent_id;
        }

        if (matrixId >= mdl.mDrawMatrices.size()) {
          mdl.mDrawMatrices.resize(matrixId + 1);
          mdl.mDrawMatrices[matrixId].mWeights.emplace_back(desc->boneId, 1.0f);
        }
      }
      // Either-or: A matrix is either single-bound (EVP) or multi-influence
      // (NODEMIX)
      else if (auto* evp =
                   std::get_if<ByteCodeLists::EnvelopeMatrix>(&command)) {
        auto& drws = mdl.mDrawMatrices;
        if (drws.size() <= evp->mtxId) {
          drws.resize(evp->mtxId + 1);
        }
        drws[evp->mtxId].mWeights.clear();
        drws[evp->mtxId].mWeights.emplace_back(evp->nodeId, 1.0f);
      } else if (auto* mix = std::get_if<ByteCodeLists::NodeMix>(&command)) {
        auto& drws = mdl.mDrawMatrices;
        if (drws.size() <= mix->mtxId) {
          drws.resize(mix->mtxId + 1);
        }
        drws[mix->mtxId].mWeights.clear();
        drws[mix->mtxId].mWeights.resize(mix->blendMatrices.size());
        for (u16 i = 0; i < mix->blendMatrices.size(); ++i) {
          auto& blend = mix->blendMatrices[i];

          // TODO: Accelerate with bone table for constant-time lookup?
          auto it = std::find_if(
              binary_model.bones.begin(), binary_model.bones.end(),
              [blend](auto& bone) { return bone.matrixId == blend.mtxId; });
          assert(it != binary_model.bones.end());

          drws[mix->mtxId].mWeights[i] = libcube::DrawMatrix::MatrixWeight(
              static_cast<u32>(it - binary_model.bones.begin()), blend.ratio);
        }
      }
      // TODO: Other bytecodes
    }
  }

  // Recompute parent-child relationships
  for (size_t i = 0; i < mdl.getBones().size(); ++i) {
    auto& bone = mdl.getBones()[i];
    if (bone.mParent == -1) {
      continue;
    }
    auto& parent = mdl.getBones()[bone.mParent];
    parent.mChildren.push_back(i);
  }
}

} // namespace riistudio::g3d
