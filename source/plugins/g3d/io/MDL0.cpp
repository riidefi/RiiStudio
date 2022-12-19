#include "Common.hpp"
#include <core/common.h>
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

namespace riistudio::g3d {

void BuildRenderLists(const Model& mdl,
                      std::vector<librii::g3d::ByteCodeMethod>& renderLists) {
  librii::g3d::ByteCodeMethod nodeTree{.name = "NodeTree"};
  librii::g3d::ByteCodeMethod nodeMix{.name = "NodeMix"};
  librii::g3d::ByteCodeMethod drawOpa{.name = "DrawOpa"};
  librii::g3d::ByteCodeMethod drawXlu{.name = "DrawXlu"};

  for (size_t i = 0; i < mdl.getBones().size(); ++i) {
    for (const auto& draw : mdl.getBones()[i].mDisplayCommands) {
      librii::g3d::ByteCodeLists::Draw cmd{
          .matId = static_cast<u16>(draw.mMaterial),
          .polyId = static_cast<u16>(draw.mPoly),
          .boneId = static_cast<u16>(i),
          .prio = draw.mPrio,
      };
      bool xlu = draw.mMaterial < std::size(mdl.getMaterials()) &&
                 mdl.getMaterials()[draw.mMaterial].xlu;
      (xlu ? &drawXlu : &drawOpa)->commands.push_back(cmd);
    }
    auto parent = mdl.getBones()[i].mParent;
    assert(parent < std::ssize(mdl.getBones()));
    librii::g3d::ByteCodeLists::NodeDescendence desc{
        .boneId = static_cast<u16>(i),
        .parentMtxId =
            static_cast<u16>(parent >= 0 ? mdl.getBones()[parent].matrixId : 0),
    };
    nodeTree.commands.push_back(desc);
  }

  auto write_drw = [&](const libcube::DrawMatrix& drw, size_t i) {
    if (drw.mWeights.size() > 1) {
      librii::g3d::ByteCodeLists::NodeMix mix{
          .mtxId = static_cast<u16>(i),
      };
      for (auto& weight : drw.mWeights) {
        assert(weight.boneId < mdl.getBones().size());
        mix.blendMatrices.push_back(
            librii::g3d::ByteCodeLists::NodeMix::BlendMtx{
                .mtxId =
                    static_cast<u16>(mdl.getBones()[weight.boneId].matrixId),
                .ratio = weight.weight,
            });
      }
      nodeMix.commands.push_back(mix);
    } else {
      assert(drw.mWeights[0].boneId < mdl.getBones().size());
      librii::g3d::ByteCodeLists::EnvelopeMatrix evp{
          .mtxId = static_cast<u16>(i),
          .nodeId = static_cast<u16>(drw.mWeights[0].boneId),
      };
      nodeMix.commands.push_back(evp);
    }
  };

  // TODO: Better heuristic. When do we *need* NodeMix? Presumably when at least
  // one bone is weighted to a matrix that is not a bone directly? Or when that
  // bone is influenced by another bone?
  bool needs_nodemix = false;
  for (auto& mtx : mdl.mDrawMatrices) {
    if (mtx.mWeights.size() > 1) {
      needs_nodemix = true;
      break;
    }
  }

  if (needs_nodemix) {
    // Bones come first
    for (auto& bone : mdl.getBones()) {
      auto& drw = mdl.mDrawMatrices[bone.matrixId];
      write_drw(drw, bone.matrixId);
    }
    for (size_t i = 0; i < mdl.mDrawMatrices.size(); ++i) {
      auto& drw = mdl.mDrawMatrices[i];
      if (drw.mWeights.size() == 1) {
        // Written in pre-pass
        continue;
      }
      write_drw(drw, i);
    }
  }

  renderLists.push_back(nodeTree);
  if (!nodeMix.commands.empty()) {
    renderLists.push_back(nodeMix);
  }
  if (!drawOpa.commands.empty()) {
    renderLists.push_back(drawOpa);
  }
  if (!drawXlu.commands.empty()) {
    renderLists.push_back(drawXlu);
  }
}
librii::g3d::BinaryModel toBinaryModel(const Model& mdl) {
  librii::g3d::BinaryModel bin;

  bin.bones = {mdl.getBones().begin(), mdl.getBones().end()};
  bin.materials = {mdl.getMaterials().begin(), mdl.getMaterials().end()};
  bin.meshes = {mdl.getMeshes().begin(), mdl.getMeshes().end()};
  bin.positions = {mdl.getBuf_Pos().begin(), mdl.getBuf_Pos().end()};
  bin.normals = {mdl.getBuf_Nrm().begin(), mdl.getBuf_Nrm().end()};
  bin.colors = {mdl.getBuf_Clr().begin(), mdl.getBuf_Clr().end()};
  bin.texcoords = {mdl.getBuf_Uv().begin(), mdl.getBuf_Uv().end()};
  // Compute ModelInfo
  {
    std::set<s16> displayMatrices =
        librii::gx::computeDisplayMatricesSubset(mdl.getMeshes());
    const auto [nVert, nTri] = computeVertTriCounts(mdl);
    bool nrmMtx = std::any_of(mdl.getMeshes().begin(), mdl.getMeshes().end(),
                              [](auto& m) { return m.needsNormalMtx(); });
    bool texMtx = std::any_of(mdl.getMeshes().begin(), mdl.getMeshes().end(),
                              [](auto& m) { return m.needsTextureMtx(); });
    librii::g3d::BinaryModelInfo info{
        .scalingRule = mdl.mScalingRule,
        .texMtxMode = mdl.mTexMtxMode,
        .numVerts = nVert,
        .numTris = nTri,
        .sourceLocation = mdl.sourceLocation,
        .numViewMtx = static_cast<u32>(displayMatrices.size()),
        .normalMtxArray = nrmMtx,
        .texMtxArray = texMtx,
        .boundVolume = false,
        .evpMtxMode = mdl.mEvpMtxMode,
        .min = mdl.aabb.min,
        .max = mdl.aabb.max,
    };
    {
      // Matrix -> Bone LUT
      auto& lut = info.mtxToBoneLUT.mtxIdToBoneId;
      lut.resize(mdl.mDrawMatrices.size());
      for (size_t i = 0; i < mdl.mDrawMatrices.size(); ++i) {
        auto& mtx = mdl.mDrawMatrices[i];
        bool multi_influence = mtx.mWeights.size() > 1;
        s32 boneId = -1;
        if (!multi_influence) {
          auto it =
              std::find_if(mdl.getBones().begin(), mdl.getBones().end(),
                           [i](auto& bone) { return bone.matrixId == i; });
          if (it != mdl.getBones().end()) {
            boneId = it - mdl.getBones().begin();
          }
        }
        lut[i] = boneId;
      }
    }
    bin.info = info;
  }
  bin.name = mdl.mName;
  BuildRenderLists(mdl, bin.bytecodes);

  return bin;
}

} // namespace riistudio::g3d
