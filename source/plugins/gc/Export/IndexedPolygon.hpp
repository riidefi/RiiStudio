#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <librii/gx.h>

namespace libcube {

class Model;

struct IndexedPolygon : public riistudio::lib3d::Polygon {
  virtual void setId(u32 id) = 0;

  virtual librii::gx::MeshData& getMeshData() = 0;
  virtual const librii::gx::MeshData& getMeshData() const = 0;

  std::expected<riistudio::lib3d::IndexRange, std::string>
  propagate(const riistudio::lib3d::Model& mdl, u32 mp_id,
            librii::glhelper::VBOBuilder& out) const override;
  virtual std::span<const glm::vec3> getPos(const Model& mdl) const = 0;
  virtual std::span<const glm::vec3> getNrm(const Model& mdl) const = 0;
  virtual std::span<const librii::gx::Color> getClr(const Model& mdl,
                                                    u64 chan) const = 0;
  virtual std::span<const glm::vec2> getUv(const Model& mdl,
                                           u64 chan) const = 0;

  virtual u64 addPos(libcube::Model& mdl, const glm::vec3& v) = 0;
  virtual u64 addNrm(libcube::Model& mdl, const glm::vec3& v) = 0;
  virtual u64 addClr(libcube::Model& mdl, u64 chan, const glm::vec4& v) = 0;
  virtual u64 addUv(libcube::Model& mdl, u64 chan, const glm::vec2& v) = 0;

  void update() override {
    // Split up added primitives if necessary
  }

  virtual librii::gx::VertexDescriptor& getVcd() {
    return getMeshData().mVertexDescriptor;
  }
  virtual const librii::gx::VertexDescriptor& getVcd() const {
    return getMeshData().mVertexDescriptor;
  }

  virtual void init(bool skinned, librii::math::AABB* boundingBox) = 0;
  virtual void initBufsFromVcd(riistudio::lib3d::Model&) {}
};

template <typename T> struct SafeIndexer {
  SafeIndexer() = default;
  SafeIndexer(std::span<T> s) : span(s) {}
  std::expected<T, std::string> get(ptrdiff_t index) {
    if (index >= std::ssize(span)) [[unlikely]] {
      return std::unexpected(std::format("Index {} exceeds buffer size of {}",
                                         index, span.size()));
    }
    if (index < 0) [[unlikely]] {
      return std::unexpected(std::format("Index {} is subzero", index));
    }
    return span[index];
  }
  auto operator[](ptrdiff_t index) { return get(index); }
  std::span<T> span;
};

struct PolyIndexer {
  PolyIndexer(const IndexedPolygon& p, const libcube::Model& gmdl)
      : positions(p.getPos(gmdl)), normals(p.getNrm(gmdl)) {
    auto vcd = p.getMeshData().mVertexDescriptor;
    for (size_t i = 0; i < 2; ++i) {
      if (vcd[static_cast<librii::gx::VertexAttribute>(
              static_cast<int>(librii::gx::VertexAttribute::Color0) + i)])
        colors[i] = SafeIndexer{p.getClr(gmdl, i)};
    }
    for (size_t i = 0; i < 8; ++i) {
      if (vcd[static_cast<librii::gx::VertexAttribute>(
              static_cast<int>(librii::gx::VertexAttribute::TexCoord0) + i)])
        uvs[i] = SafeIndexer{p.getUv(gmdl, i)};
    }
  }
  SafeIndexer<const glm::vec3> positions;
  SafeIndexer<const glm::vec3> normals;
  std::array<SafeIndexer<const librii::gx::Color>, 2> colors;
  std::array<SafeIndexer<const glm::vec2>, 8> uvs;
};

} // namespace libcube
