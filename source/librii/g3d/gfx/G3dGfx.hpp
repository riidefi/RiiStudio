#pragma once

#include <librii/gfx/SceneNode.hpp>
#include <memory>
#include <plugins/g3d/collection.hpp>

#include <librii/gfx/SceneState.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/gl/EnumConverter.hpp>
#include <librii/glhelper/GlTexture.hpp>
#include <librii/glhelper/ShaderProgram.hpp>
#include <unordered_map>
#include <variant>

namespace librii::g3d::gfx {

using namespace riistudio;

// Add a Mesh to a VBO, returning the indices corresponding to that mesh.
//
// - Assumes poly.propagate(...) adds to the end of the VBO.
//
// TODO: poly.propagate is a hot path
inline std::expected<lib3d::IndexRange, std::string>
AddPolygonToVBO(librii::glhelper::VBOBuilder& vbo_builder,
                const riistudio::lib3d::Model& mdl,
                const libcube::IndexedPolygon& poly, u32 mp_id) {
  return poly.propagate(mdl, mp_id, vbo_builder);
}

struct ShaderCompileError {
  std::string desc;
};

inline std::variant<librii::glhelper::ShaderProgram, ShaderCompileError>
CompileMaterial(const lib3d::Material& _mat) {
  DebugReport("Compiling shader for %s..\n", _mat.getName().c_str());
  const auto shader_sources_ = _mat.generateShaders();
  if (!shader_sources_.has_value()) {
    _mat.isShaderError = true;
    _mat.shaderError =
        std::format("ShaderGen Error: {}", shader_sources_.error());
    return ShaderCompileError{.desc = _mat.shaderError};
  }
  auto shader_sources = *shader_sources_;
  librii::glhelper::ShaderProgram new_shader(
      shader_sources.first,
      _mat.applyCacheAgain ? _mat.cachedPixelShader : shader_sources.second);
  if (new_shader.getError()) {
    _mat.isShaderError = true;
    _mat.shaderError = std::format("GLSL Error: {}", new_shader.getErrorDesc());
    return ShaderCompileError{.desc = _mat.shaderError};
  }
  _mat.isShaderError = false;

  return new_shader;
}

struct CompiledLib3dTexture {
  CompiledLib3dTexture() = default;
  CompiledLib3dTexture(const lib3d::Texture& tex)
      : cached_gl_texture(tex), cached_generation_id(tex.getGenerationId()) {}

  void forceInvalidate(const lib3d::Texture& tex) {
    cached_gl_texture = librii::glhelper::GlTexture{tex};
    cached_generation_id = tex.getGenerationId();
  }

  void update(const lib3d::Texture& tex) {
    if (cached_generation_id != tex.getGenerationId())
      forceInvalidate(tex);
  }

  u32 getGlId() const { return cached_gl_texture.getGlId(); }

  librii::glhelper::GlTexture cached_gl_texture;
  lib3d::GenerationIDTracked::GenerationID cached_generation_id;
};

// Texture cache, by texture name alone
struct G3dTextureCache {
  // Maps texture names -> GL id
  std::map<std::string, CompiledLib3dTexture> mTexIdMap;

  bool isCached(const lib3d::Texture& tex) const {
    return mTexIdMap.contains(tex.getName());
  }

  Result<void> cache(const lib3d::Texture& tex) {
    mTexIdMap[tex.getName()] = tex;
    return {};
  }

  void invalidate() { mTexIdMap.clear(); }

  std::optional<u32> getCachedTexture(const std::string& tex) {
    if (!mTexIdMap.contains(tex))
      return std::nullopt;

    return mTexIdMap.at(tex).getGlId();
  }

  std::expected<u32, std::string> getCachedTexture(const lib3d::Texture& tex) {
    if (!isCached(tex))
      TRY(cache(tex));

    assert(isCached(tex) && "Internal logic error. cache() should either "
                            "succeed or return unexpected");
    return mTexIdMap[tex.getName()].getGlId();
  }

  void update(const lib3d::Scene& host) {
    std::set<std::string> updated;
    for (auto& tex : host.getTextures()) {
      updated.emplace(tex.getName());
      if (isCached(tex)) {
        // Possibly reupload data if the generation ID has changed.
        mTexIdMap[tex.getName()].update(tex);
        continue;
      }

      cache(tex);
    }

    // Bust old data from cache
    std::vector<std::string> old;
    for (auto& entry : mTexIdMap) {
      if (!updated.contains(entry.first)) {
        old.push_back(entry.first);
      }
    }
    for (auto& entry : old) {
      mTexIdMap.erase(entry);
    }
  }
};

// Holds a ShaderProgram in a heap allocated box at a steady address; this
// allows it to be attached to a material as a callback for updates.
struct ObservableShader {
  ObservableShader(librii::glhelper::ShaderProgram&& shader) {
    mImpl = std::make_unique<Impl>(std::move(shader));
  }

  std::expected<librii::glhelper::ShaderProgram*, std::string> getProgram() {
    if (mImpl->mErr.size()) {
      return std::unexpected(mImpl->mErr);
    }
    return &mImpl->mProgram;
  }
  void syncWithMaterial(const lib3d::Material& mat) { mImpl->update(&mat); }

private:
  // IObservers should be heap allocated
  struct Impl {
    Impl(librii::glhelper::ShaderProgram&& program)
        : mProgram(std::move(program)) {}

    librii::glhelper::ShaderProgram mProgram;
    s32 mLastId = 0xFFFF'FFFF;
    std::string mErr;

    void update(const lib3d::Material* _mat) {
      assert(_mat != nullptr);
      if (mLastId == _mat->getGenerationId())
        return;
      mLastId = _mat->getGenerationId();

      auto result = CompileMaterial(*_mat);
      if (auto* shader = std::get_if<librii::glhelper::ShaderProgram>(&result);
          shader != nullptr) {
        mProgram = std::move(*shader);
        mErr = {};
        return;
      }

      if (auto* err = std::get_if<ShaderCompileError>(&result);
          err != nullptr) {
        mErr = err->desc;
      }
    }
  };
  std::unique_ptr<Impl> mImpl;
};

// Name-based shader cache with observables
struct GenericShaderCache_WithObserverUpdates {
  // Maps material name -> Shader
  // Each entry is heap allocated so we shouldnt have to worry about dangling
  // references.
  std::map<std::string, ObservableShader> mMatToShader;

  // TOOD: Better approach than observers
  bool isShaderCached(const lib3d::Material& mat) {
    return mMatToShader.contains(mat.getName());
  }

  Result<void> cacheShader(const lib3d::Material& mat) {
    const auto shader_sources = TRY(mat.generateShaders());

    mMatToShader.emplace(
        mat.getName(), librii::glhelper::ShaderProgram{shader_sources.first,
                                                       shader_sources.second});
    mMatToShader.at(mat.getName()).syncWithMaterial(mat);
    return {};
  }

  // TODO: We could do this all with just a map type
  std::expected<librii::glhelper::ShaderProgram*, std::string>
  getCachedShader(const lib3d::Material& mat) {
    if (!isShaderCached(mat)) {
      TRY(cacheShader(mat));
    }

    assert(isShaderCached(mat));
    mMatToShader.at(mat.getName()).syncWithMaterial(mat);
    auto program = mMatToShader.at(mat.getName()).getProgram();
    if (!program) {
      return std::unexpected(program.error());
    }
    return program;
  }
};

struct G3dShaderCache_WithUnusableHashingMechanism {
  using MatData = librii::g3d::G3dMaterialData;

  class MatDataHash {
  public:
    std::size_t operator()(const MatData& data) const {
      // We can do a lot better
      return std::hash<std::string>()(data.name);
    }
  };

  std::unordered_map<MatData, std::optional<librii::glhelper::ShaderProgram>,
                     MatDataHash>
      data;

  Result<librii::glhelper::ShaderProgram*>
  getCachedShader(const riistudio::g3d::Material& mat) {
    // TODO: This could be more optimized
    auto it = data.find(mat);
    if (it != data.end())
      return &*it->second;

    auto result = CompileMaterial(mat);

    if (auto* shader = std::get_if<librii::glhelper::ShaderProgram>(&result);
        shader != nullptr) {
      return &*(data[mat] = std::move(*shader));
    }

    // Compilation failed, so give up and pick a different shader
    if (data.size()) {
      return &*data.begin()->second;
    }

    // We can't do anything
    return std::unexpected("Shader compiler error and no other shader to use");
  }
};

// GenericShaderCache_WithObserverUpdates: Hashmap ShaderData -> Shader; only
// latest version kept in-memory G3dShaderCache_WithUnusableHashingMechanism:
// Uses hashmap to store a mapping of ShaderData -> Shader

using G3dShaderCache = GenericShaderCache_WithObserverUpdates;
// using G3dShaderCache = G3dShaderCache_WithUnusableHashingMechanism;

//! Represents a unique path to a certain drawcall.
//!
//! Assumes names are unique
struct DrawCallPath {
  std::string model_name; // Model
  std::string mesh_name;  // Mesh
  u32 mprim_index = 0;    // Draw call

  bool operator==(const DrawCallPath&) const = default;
};

class DrawCallPathHash {
public:
  std::size_t operator()(const DrawCallPath& name) const {
    return std::hash<std::string>()(name.model_name) ^
           std::hash<std::string>()(name.mesh_name) ^ name.mprim_index;
  }
};

template <typename T>
using DrawCallMap = std::unordered_map<DrawCallPath, T, DrawCallPathHash>;

struct G3dVertexRenderData {
  //
  // WARNING: mVboBuilder is directly used
  //
  librii::glhelper::VBOBuilder mVboBuilder;
  // Maps a draw call -> ranges of mVboBuilder
  DrawCallMap<lib3d::IndexRange> mTenants;

  std::expected<lib3d::IndexRange, std::string>
  getDrawCallVertices(const DrawCallPath& path) const {
    if (!mTenants.contains(path)) {
      return std::unexpected(
          std::format("mTenants does not contain (model:{}, mesh:{}, mprim:{})",
                      path.model_name, path.mesh_name, path.mprim_index));
    }
    return mTenants.at(path);
  }

  Result<void> buildVertexBuffer(const lib3d::Model& model) {
    for (auto& mesh : model.getMeshes()) {
      auto& gc_mesh = reinterpret_cast<const libcube::IndexedPolygon&>(mesh);

      for (u32 i = 0; i < gc_mesh.getMeshData().mMatrixPrimitives.size(); ++i) {
        const DrawCallPath mesh_name{.model_name = "TODO",
                                     .mesh_name = mesh.getName(),
                                     .mprim_index = i};

        if (mTenants.contains(mesh_name))
          continue;

        const auto index_range =
            TRY(AddPolygonToVBO(mVboBuilder, model, gc_mesh, i));
        mTenants.emplace(mesh_name, index_range);
      }
    }
    return {};
  }

  Result<void> init(const lib3d::Scene& host) {
    for (auto& model : host.getModels()) {
      TRY(buildVertexBuffer(model));
    }

    TRY(mVboBuilder.build());
    return {};
  }
};

// - One vertex buffer object (VBO) representing the entire model
// (librii::glhelper::VBOBuilder)
// - A mapping of draw calls in the model to indices in the VBO
// - A list of GL texture objects
// - A mapping of .brres textures to slots of GL texture objects
// - A list+mapping of material names to observable GL shader objects
struct G3dSceneRenderData {
  G3dVertexRenderData mVertexRenderData;
  G3dTextureCache mTextureData;
  G3dShaderCache mMaterialData;

  Result<void> init(const lib3d::Scene& host) {
    TRY(mVertexRenderData.init(host));
    mTextureData.update(host);
    // Shaders will be generated the first time the scene is drawn
    return {};
  }
};

// This only needs to be created once
//
std::unique_ptr<G3dSceneRenderData>
G3DSceneCreateRenderData(riistudio::g3d::Collection& scene);

Result<void> G3DSceneAddNodesToBuffer(riistudio::lib3d::SceneState& state,
                                      const riistudio::g3d::Collection& scene,
                                      glm::mat4 v_mtx, glm::mat4 p_mtx,
                                      G3dSceneRenderData& render_data);

Result<void> Any3DSceneAddNodesToBuffer(riistudio::lib3d::SceneState& state,
                                        const lib3d::Scene& scene,
                                        glm::mat4 v_mtx, glm::mat4 p_mtx,
                                        G3dSceneRenderData& render_data);

} // namespace librii::g3d::gfx
