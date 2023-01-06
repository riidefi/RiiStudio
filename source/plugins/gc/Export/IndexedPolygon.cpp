#include "IndexedPolygon.hpp"
#include <librii/gl/Compiler.hpp>

// For some reason I cannot comprehend, we need this to fix linking on Linux:
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
#include <librii/gl/Compiler.cpp>
#include <librii/mtx/TexMtx.cpp>
#endif

#include <vendor/magic_enum/magic_enum.hpp>

namespace libcube {

bool IndexedPolygon::hasAttrib(SimpleAttrib attrib) const {
  switch (attrib) {
  case SimpleAttrib::EnvelopeIndex:
    return getVcd()[gx::VertexAttribute::PositionNormalMatrixIndex];
  case SimpleAttrib::Position:
    return getVcd()[gx::VertexAttribute::Position];
  case SimpleAttrib::Normal:
    return getVcd()[gx::VertexAttribute::Position];
  case SimpleAttrib::Color0:
  case SimpleAttrib::Color1:
    return getVcd()[gx::VertexAttribute::Color0 +
                    ((u64)attrib - (u64)SimpleAttrib::Color0)];
  case SimpleAttrib::TexCoord0:
  case SimpleAttrib::TexCoord1:
  case SimpleAttrib::TexCoord2:
  case SimpleAttrib::TexCoord3:
  case SimpleAttrib::TexCoord4:
  case SimpleAttrib::TexCoord5:
  case SimpleAttrib::TexCoord6:
  case SimpleAttrib::TexCoord7:
    return getVcd()[gx::VertexAttribute::TexCoord0 +
                    ((u64)attrib - (u64)SimpleAttrib::TexCoord0)];
  default:
    return false;
  }
}
void IndexedPolygon::setAttrib(SimpleAttrib attrib, bool v) {
  // We usually recompute the index (no direct support*)
  switch (attrib) {
  case SimpleAttrib::EnvelopeIndex:
    getVcd().mAttributes[gx::VertexAttribute::PositionNormalMatrixIndex] =
        gx::VertexAttributeType::Direct;
    getVcd().mBitfield |=
        (1 << (int)gx::VertexAttribute::PositionNormalMatrixIndex);
    break;
  case SimpleAttrib::Position:
    getVcd().mAttributes[gx::VertexAttribute::Position] =
        gx::VertexAttributeType::Short;
    getVcd().mBitfield |= (1 << (int)gx::VertexAttribute::Position);
    break;
  case SimpleAttrib::Normal:
    getVcd().mAttributes[gx::VertexAttribute::Normal] =
        gx::VertexAttributeType::Short;
    getVcd().mBitfield |= (1 << (int)gx::VertexAttribute::Normal);
    break;
  case SimpleAttrib::Color0:
  case SimpleAttrib::Color1:
    getVcd().mAttributes[gx::VertexAttribute::Color0 +
                         ((u64)attrib - (u64)SimpleAttrib::Color0)] =
        gx::VertexAttributeType::Short;
    getVcd().mBitfield |= (1 << ((int)gx::VertexAttribute::Color0 +
                                 ((u64)attrib - (u64)SimpleAttrib::Color0)));
    break;
  case SimpleAttrib::TexCoord0:
  case SimpleAttrib::TexCoord1:
  case SimpleAttrib::TexCoord2:
  case SimpleAttrib::TexCoord3:
  case SimpleAttrib::TexCoord4:
  case SimpleAttrib::TexCoord5:
  case SimpleAttrib::TexCoord6:
  case SimpleAttrib::TexCoord7:
    getVcd().mAttributes[gx::VertexAttribute::TexCoord0 +
                         ((u64)attrib - (u64)SimpleAttrib::TexCoord0)] =
        gx::VertexAttributeType::Short;
    getVcd().mBitfield |= (1 << ((int)gx::VertexAttribute::TexCoord0 +
                                 ((u64)attrib - (u64)SimpleAttrib::TexCoord0)));
    break;
  default:
    assert(!"Invalid simple vertex attrib");
    break;
  }
}

template <typename T> struct SafeIndexer {
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

std::expected<riistudio::lib3d::IndexRange, std::string>
IndexedPolygon::propagate(const riistudio::lib3d::Model& mdl, u32 mp_id,
                          librii::glhelper::VBOBuilder& out) const {
  riistudio::lib3d::IndexRange vertex_indices;
  vertex_indices.start = static_cast<u32>(out.mIndices.size());
  // Expand mIndices by adding vertices

  const libcube::Model& gmdl = reinterpret_cast<const libcube::Model&>(mdl);
  u32 final_bitfield = 0;

  SafeIndexer positions(getPos(gmdl));
  SafeIndexer normals(getNrm(gmdl));

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
        out.pushData(0, TRY(positions[vtx[gx::VertexAttribute::Position]]));
        break;
      case gx::VertexAttribute::Color0:
        out.pushData(5, getClr(gmdl, 0, vtx[gx::VertexAttribute::Color0]));
        break;
      case gx::VertexAttribute::Color1:
        out.pushData(6, getClr(gmdl, 1, vtx[gx::VertexAttribute::Color1]));
        break;
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
        const auto data = getUv(gmdl, chan, vtx[attr]);
        out.pushData(7 + chan, data);
        break;
      }
      case gx::VertexAttribute::Normal:
        out.pushData(4, TRY(normals[vtx[gx::VertexAttribute::Normal]]));
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
      for (const auto& v : idx.mVertices) {
        TRY(propVtx(v));
      }
      return {};
    case gx::PrimitiveType::TriangleFan: {
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
    }
    EXPECT(false, "Unexpected primitive type");
  };

  auto& mprims = getMeshData().mMatrixPrimitives;
  for (auto& idx : mprims[mp_id].mPrimitives)
    TRY(propPrim(idx));

  for (int i = 0; i < (int)gx::VertexAttribute::Max; ++i) {
    auto attr = (gx::VertexAttribute)i;
    if (!(final_bitfield & (1 << i)) &&
        i != (int)gx::VertexAttribute::PositionNormalMatrixIndex)
      continue;
    // For now, we just skip it
    if (i == (int)gx::VertexAttribute::NormalBinormalTangent)
      continue;

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
