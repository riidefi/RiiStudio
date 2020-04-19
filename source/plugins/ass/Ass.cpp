// Assimp importer.

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

#include <vendor/assimp/Importer.hpp>
#include <vendor/assimp/cimport.h>
#include <vendor/assimp/postprocess.h>
#include <vendor/assimp/scene.h>

#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/j3d/Scene.hpp>

#include <algorithm>
#include <map>
#include <vendor/stb_image.h>
#undef min

namespace riistudio::ass {

#pragma region Utility
static glm::vec3 getVec(const aiVector3D& vec) { return {vec.x, vec.y, vec.z}; }
static glm::vec2 getVec2(const aiVector3D& vec) { return {vec.x, vec.y}; }
static libcube::gx::Color getClr(const aiColor4D& clr) {
  libcube::gx::ColorF32 fclr{clr.r, clr.g, clr.b, clr.a};
  return fclr;
}
static glm::mat4 getMat4(const aiMatrix4x4& mtx) {
  glm::mat4 out;

  out[0][0] = mtx.a1;
  out[1][0] = mtx.a2;
  out[2][0] = mtx.a3;
  out[3][0] = mtx.a4;
  out[0][1] = mtx.b1;
  out[1][1] = mtx.b2;
  out[2][1] = mtx.b3;
  out[3][1] = mtx.b4;
  out[0][2] = mtx.c1;
  out[1][2] = mtx.c2;
  out[2][2] = mtx.c3;
  out[3][2] = mtx.c4;
  out[0][3] = mtx.d1;
  out[1][3] = mtx.d2;
  out[2][3] = mtx.d3;
  out[3][3] = mtx.d4;
  return out;
}

struct IdCounter {
  u32 boneId = 0;
  u32 meshId = 0;
  u32 matId = 0;

  std::map<const aiNode*, u32> nodeToBoneIdMap;
  std::map<u32, u32> matIdToMatIdMap;
};

// Actual data we can respect
enum class ImpTexType {
  Diffuse,  // Base color
  Specular, // Attenuate spec lighting or env lighting
  Ambient,  // We can't mix with amb color, but we can just add it on.
  Emissive, // We also add this on. Perhaps later in the pipeline.
  Bump,     // We use the height map as a basic bump map.
            // We do not support normal maps. Perhaps convert to Bump.
  // We do not support shininess. In setups with two env maps, we could
  // use this to attenuate the specular one. For now, though, it is
  // unsupported.
  Opacity,      // This can actually be useful. For example, combining CMPR +
                // I8.
  Displacement, // Effectively the same as bump maps, but attenuate the
                // single diffuse component.

  // Treated as second diffuse, for now
  // LightMap,     // Any second diffuse image gets moved here
  // We don't support reflection.
  // Base Color -> Diffuse
  // Normal Camera -> Unsupported
  // Emission Color -> Emissive
  // Metalness -> Ignored for now. We can tint specular to achieve this.
  // Diffuse Roughness -> Invert for specular eventually
  // AO -> ANother LightMap
  // Unknown -> Treat as diffuse, if beyond first slot, LightMap
};
struct ImpSampler {
  ImpTexType type;
  std::string path;
  u32 uv_channel;
  libcube::gx::TextureWrapMode wrap;
};
struct ImpMaterial {
  std::vector<ImpSampler> samplers;
};
void CompileMaterial(libcube::IGCMaterial& out, const ImpMaterial& in,
                     std::set<std::string>& texturesToImport) {
  // Vertex Lighting:
  // Ambient* + (Diffuse* x LightMap x DIFFUSE_LIGHT)
  // + (Specular* x SPECULAR_LIGHT) -> Replace with opacity, not attenuate

  // Env Lighting:
  // Ambient* + (Diffuse* x LightMap x DIFFUSE_TEX*(Bump)) +
  // (Specular* x SPECULAR_TEX*(Bump)) -> Replace with opacity

  // Current, lazy setup:
  // Diffuse x Diffuse.. -> Replace with opacity

  // WIP, just supply a pure color..
  auto& data = out.getMaterialData();

  if (!in.samplers.empty()) {
    data.samplers.push_back(
        std::move(std::make_unique<j3d::Material::SamplerData>()));
    data.samplers[0]->mTexture = in.samplers[0].path;
    texturesToImport.insert(in.samplers[0].path);
    data.texMatrices.push_back(
        std::move(std::make_unique<j3d::Material::TexMatrix>()));
    data.texGens.push_back({});
  }
  libcube::gx::TevStage wip;
  wip.texMap = wip.texCoord = 0;
  wip.colorStage.d = libcube::gx::TevColorArg::texc;
  wip.alphaStage.d = libcube::gx::TevAlphaArg::texa;
  data.tevColors[0] = {0xaa, 0xbb, 0xcc, 0xff};
  data.shader.mStages[0] = wip;
}
std::tuple<aiString, unsigned int, aiTextureMapMode>
GetTexture(aiMaterial* pMat, int t, int j) {
  aiString path;
  // No support for anything non-uv -- already processed away by the time
  // we get it aiTextureMapping mapping;
  unsigned int uvindex = 0;
  // We don't support blend/op.
  // All the data here *could* be translated to TEV,
  // but it isn't practical.
  // ai_real blend;
  // aiTextureOp op;
  // No decal support
  u32 mapmode = aiTextureMapMode_Wrap;

  pMat->GetTexture(static_cast<aiTextureType>(t), j, &path, nullptr, &uvindex,
                   nullptr, nullptr, nullptr //(aiTextureMapMode*)&mapmode
  );
  return {path, uvindex, (aiTextureMapMode)mapmode};
}
#pragma endregion

constexpr libcube::gx::VertexAttribute PNM =
    libcube::gx::VertexAttribute::PositionNormalMatrixIndex;

struct AssImporter {
  IdCounter ctr;
  IdCounter* boneIdCtr = &ctr;

  const aiScene* pScene = nullptr;
  j3d::CollectionAccessor out_collection;
  j3d::ModelAccessor out_model_ac;
  j3d::Model& out_model;

  AssImporter(const aiScene* scene, kpi::IDocumentNode* mdl)
      : pScene(scene), out_collection({mdl}),
        out_model_ac(out_collection.addModel()), out_model(out_model_ac.get()) {
  }

  int get_bone_id(const aiNode* pNode) {
    return boneIdCtr->nodeToBoneIdMap.find(pNode) !=
                   boneIdCtr->nodeToBoneIdMap.end()
               ? boneIdCtr->nodeToBoneIdMap[pNode]
               : -1;
  }
  // Only call if weighted
  u16 add_weight_matrix_low(const j3d::DrawMatrix& drw) {
    const auto found = std::find(out_model.mDrawMatrices.begin(),
                                 out_model.mDrawMatrices.end(), drw);
    if (found != out_model.mDrawMatrices.end()) {
      return found - out_model.mDrawMatrices.begin();
    } else {
      out_model.mDrawMatrices.push_back(drw);
      return out_model.mDrawMatrices.size() - 1;
    }
  }
  u16 add_weight_matrix(int v, const aiMesh* pMesh,
                        j3d::DrawMatrix* pDrwOut = nullptr) {
    j3d::DrawMatrix drw;
    for (int j = 0; j < pMesh->mNumBones; ++j) {
      const auto* pBone = pMesh->mBones[j];

      for (int k = 0; k < pBone->mNumWeights; ++k) {
        const auto* pWeight = &pBone->mWeights[k];
        if (pWeight->mVertexId == v) {
          const auto boneid = get_bone_id(pBone->mNode);
          assert(boneid != -1);
          drw.mWeights.emplace_back(boneid, pWeight->mWeight);
          break;
        }
      }
    }
    if (pDrwOut != nullptr)
      *pDrwOut = drw;
    return add_weight_matrix_low(drw);
  };

  void
  ProcessMeshTrianglesStatic(const aiNode* singleInfluence,
                             j3d::Shape& poly_data,
                             std::vector<libcube::IndexedVertex>&& vertices) {
    auto& mp = poly_data.mMatrixPrimitives[poly_data.addMatrixPrimitive()];
    // Copy triangle data
    // We will do triangle-stripping in a post-process
    auto& tris = mp.mPrimitives.emplace_back();
    tris.mType = libcube::gx::PrimitiveType::Triangles;
    tris.mVertices = std::move(vertices);

    const int boneId = get_bone_id(singleInfluence);
    assert(boneId > 0);
    j3d::DrawMatrix drw{{{static_cast<u32>(boneId), 1.0f}}};
    const auto mtx = add_weight_matrix_low(drw);
    mp.mCurrentMatrix = mtx;
    mp.mDrawMatrixIndices.push_back(mtx);
  }
  void
  ProcessMeshTrianglesWeighted(j3d::Shape& poly_data,
                               std::vector<libcube::IndexedVertex>&& vertices) {

    // At this point, the mtx index of vertices is global.
    // We need to convert it to a local palette index.
    // Tolerence should be implemented here, eventually.

    std::vector<u16> matrix_indices;

    for (auto& v : vertices) {
      const auto mtx_idx = v[PNM];

      // Hack: We will come across data as such (assume 3-wide sweep)
      // (A B B) (C A D)
      // To prevent this transforming into
      // (A B C) (D)
      // Where references to A in the second sweep will fail,
      // we simply don't compress across sweeps:
      // (A B B) (C A D)
      // (A B C) (A D)
      // It's far from optimal, but the current choice:
      const std::size_t sweep_id = matrix_indices.size() / 10;
      const std::size_t sweep_begin = sweep_id * 10;
      const std::size_t sweep_end =
          std::min(matrix_indices.size(), (sweep_id + 1) * 10);
      const auto found = std::find(matrix_indices.begin() + sweep_begin,
                                   matrix_indices.begin() + sweep_end, mtx_idx);

      if (found == matrix_indices.end()) {
        matrix_indices.push_back(mtx_idx);
      }
    }

    auto vertex_sweep_index = [&](int v, int sweep) -> int {
      const auto found =
          std::find(matrix_indices.begin() + sweep * 10,
                    matrix_indices.begin() +
                        std::min(static_cast<int>(matrix_indices.size()),
                                 (sweep + 1) * 10),
                    vertices[v][PNM]);

      if (found == matrix_indices.end())
        return -1;
      const auto idx = std::distance(matrix_indices.begin(), found);
      // Old code not based on sweep-specific search
      // if (idx >= sweep * 10 && idx < (sweep + 1) * 10) {
      //   // We're inside the sweep;
      //   return idx - sweep * 10;
      // }
      // We're outside the sweep:
      // return -1;
      return idx;
    };
    assert(vertices.size() % 3 == 0);
    int sweep_wave = 0;
    bool reversed = false;
    int last_sweep_vtx = 0;
    for (int f = 0; f < vertices.size() / 3; ++f) {
      const int v0 = f * 3 + 0;
      const int v1 = f * 3 + 1;
      const int v2 = f * 3 + 2;

      const int s0 = vertex_sweep_index(v0, sweep_wave);
      const int s1 = vertex_sweep_index(v1, sweep_wave);
      const int s2 = vertex_sweep_index(v2, sweep_wave);

      if (s0 < 0 || s1 < 0 || s2 < 0) {
        assert(s0 != -2 && s1 != -2 && s2 != -2 &&
               "matrix_indices has not been properly filled");

        // But submit what we have so far, first:
        auto& mp = poly_data.mMatrixPrimitives[poly_data.addMatrixPrimitive()];
        auto& tris = mp.mPrimitives.emplace_back();
        tris.mType = libcube::gx::PrimitiveType::Triangles;
        std::copy(vertices.begin() + last_sweep_vtx, vertices.begin() + v0,
                  std::back_inserter(tris.mVertices));
        last_sweep_vtx = v0;
        mp.mCurrentMatrix = -1;
        const auto sweep_begin = sweep_wave * 10;
        const auto sweep_end = std::min(
            (sweep_wave + 1) * 10, static_cast<int>(matrix_indices.size()));

        std::copy(matrix_indices.begin() + sweep_begin,
                  matrix_indices.begin() + sweep_end,
                  std::back_inserter(mp.mDrawMatrixIndices));

        // Reverse one step, but in the next wave:
        assert(!reversed && "matrix_indices is unsorted");
        --f;
        ++sweep_wave;
        reversed = true;

        continue;
      }

      reversed = false;
      vertices[v0][PNM] = s0;
      vertices[v0][PNM] = s1;
      vertices[v0][PNM] = s2;
    }
  }

  void ProcessMeshTriangles(j3d::Shape& poly_data, const aiMesh* pMesh,
                            const aiNode* pNode,
                            std::vector<libcube::IndexedVertex>&& vertices) {
    // Determine if we need to do matrix processing
    if (poly_data.mVertexDescriptor.mBitfield & (1 << (int)PNM)) {
      ProcessMeshTrianglesWeighted(poly_data, std::move(vertices));
    } else {
      // If one bone, bind to that; otherwise, bind to the node itself.
      const aiNode* single_influence =
          pMesh->HasBones() ? pMesh->mBones[0]->mNode : pNode;
      ProcessMeshTrianglesStatic(single_influence, poly_data,
                                 std::move(vertices));
    }
  }

  void ImportMesh(const aiMesh* pMesh, const aiNode* pNode) {
    assert(pMesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

    auto poly = out_model_ac.addShape();
    auto& poly_data = poly.get();
    poly_data.mode = j3d::ShapeData::Mode::Skinned;
    poly.get().bbox.min = {pMesh->mAABB.mMin.x, pMesh->mAABB.mMin.y,
                           pMesh->mAABB.mMin.z};
    poly.get().bbox.max = {pMesh->mAABB.mMax.x, pMesh->mAABB.mMax.y,
                           pMesh->mAABB.mMax.z};
    // TODO: Sphere
    auto add_attribute = [&](auto type, bool direct = false) {
      poly_data.mVertexDescriptor.mAttributes[type] =
          direct ? libcube::gx::VertexAttributeType::Direct
                 : libcube::gx::VertexAttributeType::Short;
    };
    add_attribute(libcube::gx::VertexAttribute::Position);
    if (pMesh->HasNormals())
      add_attribute(libcube::gx::VertexAttribute::Normal);

    for (int j = 0; j < 2; ++j) {
      if (pMesh->HasVertexColors(j))
        add_attribute(libcube::gx::VertexAttribute::Color0 + j);
    }
    for (int j = 0; j < 8; ++j) {
      if (pMesh->HasTextureCoords(j)) {
        add_attribute(libcube::gx::VertexAttribute::TexCoord0 + j);

        assert(pMesh->mNumUVComponents[j] == 2);
      }
    }

    auto add_to_buffer = [&](const auto& entry, auto& buf) -> u16 {
      const auto found = std::find(buf.begin(), buf.end(), entry);
      if (found != buf.end()) {
        return found - buf.begin();
      }

      buf.push_back(entry);
      return buf.size() - 1;
    };

    auto add_position = [&](int v, const j3d::DrawMatrix* wt = nullptr) {
      glm::vec3 pos = getVec(pMesh->mVertices[v]);

      // If rigid, transform into bone-space
      // This assumes that meshes will not be influenced by their children? This
      // could be a bad assumption..
      if (wt != nullptr && wt->mWeights.size() == 1) {
        const auto& acting_influence =
            out_model_ac.getJointRaw(wt->mWeights[0].boneId);
        pos = glm::vec4(pos, 0) * glm::inverse(acting_influence.calcSrtMtx(
                                      &out_model_ac.getJoints()));
      }

      return add_to_buffer(pos, out_model_ac.get().mBufs.pos.mData);
    };
    auto add_normal = [&](int v) {
      return add_to_buffer(getVec(pMesh->mNormals[v]),
                           out_model_ac.get().mBufs.norm.mData);
    };
    auto add_color = [&](int v, int j) {
      return add_to_buffer(getClr(pMesh->mColors[j][v]),
                           out_model_ac.get().mBufs.color[j].mData);
    };
    auto add_uv = [&](int v, int j) {
      return add_to_buffer(getVec2(pMesh->mTextureCoords[j][v]),
                           out_model_ac.get().mBufs.uv[j].mData);
    };

    // More than one bone -> Assume multi mtx, unless zero influence
    // With one weight, must be singlebound! no partial / null weight

    bool multi_mtx = pMesh->HasBones() && pMesh->mNumBones > 1;

    if (multi_mtx)
      add_attribute(PNM);

    poly_data.mVertexDescriptor.calcVertexDescriptorFromAttributeList();

    std::vector<libcube::IndexedVertex> vertices;

    for (int f = 0; f < pMesh->mNumFaces; ++f) {
      for (int fv = 0; fv < 3; ++fv) {
        const auto v = pMesh->mFaces[f].mIndices[fv];

        libcube::IndexedVertex vtx{};
        j3d::DrawMatrix drw;
        const auto weightInfo =
            pMesh->HasBones() ? add_weight_matrix(v, pMesh, &drw) : 0;

        if (multi_mtx) {
          vtx[PNM] = weightInfo * 3;
        }

        vtx[libcube::gx::VertexAttribute::Position] = add_position(v, &drw);
        if (pMesh->HasNormals())
          vtx[libcube::gx::VertexAttribute::Normal] = add_normal(v);
        for (int j = 0; j < 2; ++j) {
          if (pMesh->HasVertexColors(j))
            vtx[libcube::gx::VertexAttribute::Color0 + j] = add_color(v, j);
        }
        for (int j = 0; j < 8; ++j) {
          if (pMesh->HasTextureCoords(j)) {
            vtx[libcube::gx::VertexAttribute::TexCoord0 + j] = add_uv(v, j);
          }
        }
        vertices.push_back(vtx);
      }
    }

    ProcessMeshTriangles(poly_data, pMesh, pNode, std::move(vertices));
  }

  void ImportNode(const aiNode* pNode, lib3d::Bone* parent = nullptr) {
    // Create a bone (with name)
    auto joint = out_model_ac.addJoint();
    joint.get().setName(pNode->mName.C_Str());
    const glm::mat4 xf = getMat4(pNode->mTransformation);

    lib3d::SRT3 srt;
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(xf, srt.scale, rotation, srt.translation, skew, perspective);

    srt.rotation = {glm::degrees(glm::eulerAngles(rotation).x),
                    glm::degrees(glm::eulerAngles(rotation).y),
                    glm::degrees(glm::eulerAngles(rotation).z)};
    joint.get().setSRT(srt);

    IdCounter localBoneIdCtr;
    if (boneIdCtr == nullptr)
      boneIdCtr = &localBoneIdCtr;
    joint.get().id = boneIdCtr->boneId++;
    boneIdCtr->nodeToBoneIdMap[pNode] = joint.get().id;

    joint.get().setBoneParent(parent != nullptr ? parent->getId() : -1);
    if (parent != nullptr)
      parent->addChild(joint.get().getId());

    // Mesh data
    for (int i = 0; i < pNode->mNumMeshes; ++i) {
      // Can these be duplicated?
      const auto* pMesh = pScene->mMeshes[pNode->mMeshes[i]];

      assert(boneIdCtr->matIdToMatIdMap.find(pMesh->mMaterialIndex) !=
             boneIdCtr->matIdToMatIdMap.end());
      const auto matId = boneIdCtr->matIdToMatIdMap[pMesh->mMaterialIndex];
      joint.get().addDisplay({matId, boneIdCtr->meshId++, 0});

      ImportMesh(pMesh, pNode);
    }

    for (int i = 0; i < pNode->mNumChildren; ++i) {
      ImportNode(pNode->mChildren[i], &joint.get());
    }
  }

  void ImportAss() {
    const auto* root = pScene->mRootNode;
    assert(root != nullptr);

    std::set<std::string> texturesToImport;

    for (unsigned int i = 0; i < pScene->mNumMaterials; ++i) {
      auto* pMat = pScene->mMaterials[i];
      auto& mr = out_model_ac.addMaterialRaw();
      mr.id = i;
      const auto name = pMat->GetName();
      mr.name = name.C_Str();
      if (mr.name == "") {
        mr.name = "Material";
        mr.name += std::to_string(i);
      }
      boneIdCtr->matIdToMatIdMap[i] = mr.id;
      ImpMaterial impMat;

      for (int t = aiTextureType_DIFFUSE; t <= aiTextureType_UNKNOWN; ++t) {
        for (int j = 0; j < pMat->GetTextureCount((aiTextureType)t); ++j) {

          const auto [path, uvindex, mapmode] = GetTexture(pMat, t, j);

          ImpTexType impTexType = ImpTexType::Diffuse;

          switch (t) {
          case aiTextureType_DIFFUSE:
            impTexType = ImpTexType::Diffuse;
            break;
            /** The texture is combined with the result of the specular
             *  lighting equation.
             */
          case aiTextureType_SPECULAR:
            impTexType = ImpTexType::Specular;
            break;
            /** The texture is combined with the result of the ambient
             *  lighting equation.
             */
          case aiTextureType_AMBIENT:
            impTexType = ImpTexType::Ambient;
            break;
            /** The texture is added to the result of the lighting
             *  calculation. It isn't influenced by incoming light.
             */
          case aiTextureType_EMISSIVE:
            impTexType = ImpTexType::Emissive;
            break;
            /** The texture is a height map.
             *
             *  By convention, higher gray-scale values stand for
             *  higher elevations from the base height.
             */
          case aiTextureType_HEIGHT:
            impTexType = ImpTexType::Bump;
            break;
            /** The texture is a (tangent space) normal-map.
             *
             *  Again, there are several conventions for tangent-space
             *  normal maps. Assimp does (intentionally) not
             *  distinguish here.
             */
          case aiTextureType_NORMALS:
            impTexType = ImpTexType::Diffuse; // TODO
            break;
            /** The texture defines the glossiness of the material.
             *
             *  The glossiness is in fact the exponent of the specular
             *  (phong) lighting equation. Usually there is a conversion
             *  function defined to map the linear color values in the
             *  texture to a suitable exponent. Have fun.
             */
          case aiTextureType_SHININESS:
            impTexType = ImpTexType::Diffuse; // TODO
            break;
            /** The texture defines per-pixel opacity.
             *
             *  Usually 'white' means opaque and 'black' means
             *  'transparency'. Or quite the opposite. Have fun.
             */
          case aiTextureType_OPACITY:
            impTexType = ImpTexType::Opacity;
            break;
            /** Displacement texture
             *
             *  The exact purpose and format is application-dependent.
             *  Higher color values stand for higher vertex displacements.
             */
          case aiTextureType_DISPLACEMENT:
            impTexType = ImpTexType::Displacement;
            break;
            /** Lightmap texture (aka Ambient Occlusion)
             *
             *  Both 'Lightmaps' and dedicated 'ambient occlusion maps' are
             *  covered by this material property. The texture contains a
             *  scaling value for the final color value of a pixel. Its
             *  intensity is not affected by incoming light.
             */
          case aiTextureType_LIGHTMAP:
            impTexType = ImpTexType::Diffuse; // TODO
            break;
            /** Reflection texture
             *
             * Contains the color of a perfect mirror reflection.
             * Rarely used, almost never for real-time applications.
             */
          case aiTextureType_REFLECTION:
            impTexType = ImpTexType::Diffuse; // TODO
            break;
            /** PBR Materials
             * PBR definitions from maya and other modelling packages now use
             * this standard. This was originally introduced around 2012.
             * Support for this is in game engines like Godot, Unreal or
             * Unity3D. Modelling packages which use this are very common now.
             */
          case aiTextureType_BASE_COLOR:
            impTexType = ImpTexType::Diffuse;
            break;
          case aiTextureType_NORMAL_CAMERA:
            impTexType = ImpTexType::Diffuse; // TODO
            break;
          case aiTextureType_EMISSION_COLOR:
            impTexType = ImpTexType::Emissive;
            break;
          case aiTextureType_METALNESS:
            impTexType = ImpTexType::Diffuse; // TODO
            break;
          case aiTextureType_DIFFUSE_ROUGHNESS:
            impTexType = ImpTexType::Diffuse; // TODO
            break;
          case aiTextureType_AMBIENT_OCCLUSION:
            impTexType = ImpTexType::Diffuse; // LM
            break;
            /** Unknown texture
             *
             *  A texture reference that does not match any of the definitions
             *  above is considered to be 'unknown'. It is still imported,
             *  but is excluded from any further post-processing.
             */
          case aiTextureType_UNKNOWN:
            impTexType = ImpTexType::Diffuse;

            break;
          }
          libcube::gx::TextureWrapMode impWrapMode =
              libcube::gx::TextureWrapMode::Repeat;
          switch (mapmode) {
          case aiTextureMapMode_Decal:
          case aiTextureMapMode_Clamp:
            impWrapMode = libcube::gx::TextureWrapMode::Clamp;
            break;
          case aiTextureMapMode_Wrap:
            impWrapMode = libcube::gx::TextureWrapMode::Repeat;
            break;
          case aiTextureMapMode_Mirror:
            impWrapMode = libcube::gx::TextureWrapMode::Mirror;
            break;
          }

          ImpSampler impSamp{impTexType, path.C_Str(), uvindex, impWrapMode};
          impMat.samplers.push_back(impSamp);
        }
      }

      CompileMaterial(mr, impMat, texturesToImport);
    }

    for (auto& tex : texturesToImport) {
      printf("Importing texture: %s\n", tex.c_str());

      auto& data = out_collection.addTextureRaw();
      data.mName = tex;

      int width, height, channels;
      unsigned char* image =
          stbi_load((tex).c_str(), &width, &height, &channels, STBI_rgb_alpha);
      assert(image);
      data.mFormat = (int)libcube::gx::TextureFormat::CMPR;
      data.setWidth(width);
      data.setHeight(height);
      data.setMipmapCount(0);
      data.resizeData();
      data.encode(image);
      stbi_image_free(image);
    }

    ImportNode(root);
  }
};

inline bool ends_with(const std::string& value, const std::string& ending) {
  return ending.size() <= value.size() &&
         std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct AssReader {
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {

    const std::array<std::string, 3> supported_endings = {".dae", ".obj",
                                                          ".fbx"};

    if (std::find_if(supported_endings.begin(), supported_endings.end(),
                     [&](const std::string& ending) {
                       return ends_with(file, ending);
                     }) == supported_endings.end()) {
      return "";
    }

    return typeid(lib3d::Scene).name();
  }
  void read(kpi::IDocumentNode& node, oishii::BinaryReader& reader) const {
    const u32 ass_flags =
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_ValidateDataStructure | aiProcess_RemoveRedundantMaterials |
        aiProcess_PopulateArmatureData | aiProcess_FindDegenerates |
        aiProcess_FindInvalidData | aiProcess_GenUVCoords |
        aiProcess_FindInstances | aiProcess_OptimizeMeshes | aiProcess_Debone |
        aiProcess_GenBoundingBoxes | aiProcess_FlipUVs |
        aiProcess_FlipWindingOrder;

    Assimp::Importer importer;
    const auto* pScene = importer.ReadFileFromMemory(
        reader.getStreamStart(), reader.endpos(), ass_flags, reader.getFile());
    AssImporter assConverter(pScene, &node);
    assConverter.ImportAss();
  }
};
void Install() {
  auto& installer = *kpi::ApplicationPlugins::spInstance;

  installer.addDeserializer<AssReader>();
}

} // namespace riistudio::ass
