#include "IndexedPolygon.hpp"
#include <librii/gl/Compiler.hpp>

// For some reason I cannot comprehend, we need this to fix linking on Linux:
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
#include <librii/gl/Compiler.cpp>
#include <librii/mtx/TexMtx.cpp>
#endif

#include <random>
#include <vendor/magic_enum/magic_enum.hpp>

namespace libcube {

using namespace librii;

std::expected<riistudio::lib3d::IndexRange, std::string>
IndexedPolygon::propagate(const riistudio::lib3d::Model& mdl, u32 mp_id,
                          librii::glhelper::VBOBuilder& out) const {
  riistudio::lib3d::IndexRange vertex_indices;
  vertex_indices.start = static_cast<u32>(out.mIndices.size());
  // Expand mIndices by adding vertices

  const libcube::Model& gmdl = reinterpret_cast<const libcube::Model&>(mdl);
  u32 final_bitfield = 0;

  PolyIndexer indexer(*this, gmdl);

  glm::vec4 prim_id(1.0f, 1.0f, 1.0f, 1.0f);
  glm::vec4 prim_clr(1.0f, 1.0f, 1.0f, 1.0f);

  using rng = std::mt19937;
  std::uniform_int_distribution<rng::result_type> u24dist(0, 0xFF'FFFF);
  rng generator;

  auto randId = [&]() {
    u32 clr = u24dist(generator);
    prim_id.r = static_cast<float>((clr >> 16) & 0xff) / 255.0f;
    prim_id.g = static_cast<float>((clr >> 8) & 0xff) / 255.0f;
    prim_id.b = static_cast<float>((clr >> 0) & 0xff) / 255.0f;
  };

  auto propVtx = [&](const librii::gx::IndexedVertex& vtx) -> Result<void> {
    const auto& vcd = getVcd();
    out.mIndices.push_back(static_cast<u32>(out.mIndices.size()));
    EXPECT(final_bitfield == 0 || final_bitfield == vcd.mBitfield);
    final_bitfield |= vcd.mBitfield;
    // HACK:
    if (!(vcd.mBitfield &
          (1 << (u32)gx::VertexAttribute::PositionNormalMatrixIndex)))
      out.pushData(1, (float)0);
    if (!(vcd.mBitfield & (1 << (u32)gx::VertexAttribute::TexCoord0)))
      out.pushData(7, glm::vec2{});
    if (!(vcd.mBitfield & (1 << (u32)gx::VertexAttribute::TexCoord1)))
      out.pushData(8, glm::vec2{});
    if (!(vcd.mBitfield & (1 << (u32)gx::VertexAttribute::Normal)))
      out.pushData(4, glm::vec3{});
    if (!(vcd.mBitfield & (1 << (u32)gx::VertexAttribute::Color0)))
      out.pushData(5, glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
    out.pushData(15, prim_id);
    for (u32 i = 0; i < (u32)gx::VertexAttribute::Max; ++i) {
      if (!(vcd.mBitfield & (1 << i)))
        continue;

      switch (static_cast<gx::VertexAttribute>(i)) {
      case gx::VertexAttribute::PositionNormalMatrixIndex:
        out.pushData(
            1, (float)vtx[gx::VertexAttribute::PositionNormalMatrixIndex]);
        break;
      case gx::VertexAttribute::Texture0MatrixIndex:
      case gx::VertexAttribute::Texture1MatrixIndex:
      case gx::VertexAttribute::Texture2MatrixIndex:
      case gx::VertexAttribute::Texture3MatrixIndex:
      case gx::VertexAttribute::Texture4MatrixIndex:
      case gx::VertexAttribute::Texture5MatrixIndex:
      case gx::VertexAttribute::Texture6MatrixIndex:
      case gx::VertexAttribute::Texture7MatrixIndex:
        break;
      case gx::VertexAttribute::Position:
        out.pushData(
            0, TRY(indexer.positions[vtx[gx::VertexAttribute::Position]]));
        break;
      case gx::VertexAttribute::Color0: {
        auto c0 = indexer.colors[0];
        out.pushData(5, static_cast<librii::gx::ColorF32>(
                            TRY(c0[vtx[gx::VertexAttribute::Color0]])));
        break;
      }
      case gx::VertexAttribute::Color1: {
        auto c1 = indexer.colors[0];
        out.pushData(6, static_cast<librii::gx::ColorF32>(
                            TRY(c1[vtx[gx::VertexAttribute::Color1]])));
        break;
      }
      case gx::VertexAttribute::TexCoord0:
      case gx::VertexAttribute::TexCoord1:
      case gx::VertexAttribute::TexCoord2:
      case gx::VertexAttribute::TexCoord3:
      case gx::VertexAttribute::TexCoord4:
      case gx::VertexAttribute::TexCoord5:
      case gx::VertexAttribute::TexCoord6:
      case gx::VertexAttribute::TexCoord7: {
        const auto chan = i - static_cast<int>(gx::VertexAttribute::TexCoord0);
        const auto attr = static_cast<gx::VertexAttribute>(i);
        auto uvN = indexer.uvs[chan];
        const auto data = TRY(uvN[vtx[attr]]);
        out.pushData(7 + chan, data);
        break;
      }
      case gx::VertexAttribute::Normal:
        out.pushData(4, TRY(indexer.normals[vtx[gx::VertexAttribute::Normal]]));
        break;
      case gx::VertexAttribute::NormalBinormalTangent:
        break;
      default:
        EXPECT(false, "Invalid vtx attrib");
      }
    }
    return {};
  };

  auto propPrim = [&](const librii::gx::IndexedPrimitive& idx) -> Result<void> {
    auto propV = [&](int id) -> Result<void> {
      TRY(propVtx(idx.mVertices[id]));
      return {};
    };
    if (idx.mVertices.empty())
      return {};
    switch (idx.mType) {
    case gx::PrimitiveType::TriangleStrip: {
      randId();
      prim_clr = {0.0f, 1.0f, 0.0f, 1.0f};
      for (int v = 0; v < 3; ++v) {
        TRY(propV(v));
      }
      for (int v = 3; v < idx.mVertices.size(); ++v) {
        TRY(propV(v - ((v & 1) ? 1 : 2)));
        TRY(propV(v - ((v & 1) ? 2 : 1)));
        TRY(propV(v));
      }
      return {};
    }
    case gx::PrimitiveType::Triangles:
      randId();
      prim_clr = {1.0f, 0.0f, 0.0f, 0.0f};
      EXPECT(idx.mVertices.size() % 3 == 0);
      for (size_t i = 0; i < idx.mVertices.size(); i += 3) {
        randId();
        TRY(propVtx(idx.mVertices[i]));
        TRY(propVtx(idx.mVertices[i + 1]));
        TRY(propVtx(idx.mVertices[i + 2]));
      }
      return {};
    case gx::PrimitiveType::TriangleFan: {
      randId();
      prim_clr = {0.0f, 0.0f, 1.0f, 1.0f};
      for (int v = 0; v < 3; ++v) {
        TRY(propV(v));
      }
      for (int v = 3; v < idx.mVertices.size(); ++v) {
        TRY(propV(0));
        TRY(propV(v - 1));
        TRY(propV(v));
      }
      return {};
    }
    case gx::PrimitiveType::Quads:
    case gx::PrimitiveType::Quads2:
    case gx::PrimitiveType::Lines:
    case gx::PrimitiveType::LineStrip:
    case gx::PrimitiveType::Points:
      break;
    case gx::PrimitiveType::Max:
      return std::unexpected("::Max is not a valid gx::PrimitiveType");
    }
    EXPECT(false, "Unexpected primitive type");
  };

  auto& mprims = getMeshData().mMatrixPrimitives;
  for (auto& idx : mprims[mp_id].mPrimitives)
    TRY(propPrim(idx));

  for (int i = 0; i <= (int)gx::VertexAttribute::Max; ++i) {
    auto attr = (gx::VertexAttribute)i;
    if (attr != gx::VertexAttribute::Max) {
      if (!(final_bitfield & (1 << i)) &&
          i != (int)gx::VertexAttribute::PositionNormalMatrixIndex)
        continue;
      // For now, we just skip it
      if (i == (int)gx::VertexAttribute::NormalBinormalTangent)
        continue;
    }

    const auto def = TRY(librii::gl::getVertexAttribGenDef(attr));
    EXPECT(def.first != nullptr && "Internal logic error");
    if (def.first->name == nullptr) {
      return std::unexpected(
          std::format("Failed to get VertexAttributeGenDef for {} ({})",
                      magic_enum::enum_name(attr), i));
    }
    out.mPropogating[def.second].descriptor =
        librii::glhelper::VAOEntry{.binding_point = (u32)def.second,
                                   .name = def.first->name,
                                   .format = def.first->format,
                                   .size = def.first->size * 4};
  }

  vertex_indices.size =
      static_cast<u32>(out.mIndices.size()) - vertex_indices.start;
  return vertex_indices;
}
} // namespace libcube
