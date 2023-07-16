#include "RHSTImporter.hpp"

#include <LibBadUIFramework/Plugins.hpp>
#include <core/3d/i3dmodel.hpp>

#include <librii/crate/g3d_crate.hpp>
#include <librii/hx/CullMode.hpp>
#include <librii/hx/PixMode.hpp>
#include <librii/hx/TextureFilter.hpp>
#include <librii/image/CheckerBoard.hpp>
#include <librii/rhst/RHSTOptimizer.hpp>

#include <oishii/reader/binary_reader.hxx>

#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/Material.hpp>
#include <plugins/j3d/Scene.hpp>

#include <rsl/FsDialog.hpp>
#include <rsl/Stb.hpp>

#include <future>

// XXX: Hack, though we'll refactor all of this way soon
std::string rebuild_dest;

namespace riistudio::rhst {

void compileCullMode(librii::gx::LowLevelGxMaterial& mat, bool show_front,
                     bool show_back) {
  librii::hx::CullMode cm(show_front, show_back);
  mat.cullMode = cm.get();
}

librii::gx::TextureWrapMode compileWrap(librii::rhst::WrapMode in) {
  switch (in) {
  case librii::rhst::WrapMode::Clamp:
  default: // TODO
    return librii::gx::TextureWrapMode::Clamp;
  case librii::rhst::WrapMode::Repeat:
    return librii::gx::TextureWrapMode::Repeat;
  case librii::rhst::WrapMode::Mirror:
    return librii::gx::TextureWrapMode::Mirror;
  }
}

void compileAlphaMode(librii::gx::LowLevelGxMaterial& mat,
                      librii::rhst::AlphaMode alpha_mode) {
  librii::hx::PixMode pix_mode = librii::hx::PIX_DEFAULT_OPAQUE;
  switch (alpha_mode) {
  case librii::rhst::AlphaMode::Opaque:
    pix_mode = librii::hx::PIX_DEFAULT_OPAQUE;
    break;
  case librii::rhst::AlphaMode::Clip:
    pix_mode = librii::hx::PIX_STENCIL;
    break;
  case librii::rhst::AlphaMode::Translucent:
    pix_mode = librii::hx::PIX_TRANSLUCENT;
    break;
  case librii::rhst::AlphaMode::HpeStencil:
    pix_mode = librii::hx::PIX_HARRY_POTTER_STENCIL;
    break;
  case librii::rhst::AlphaMode::HpeTranslucent:
    pix_mode = librii::hx::PIX_HARRY_POTTER_TRANSLUCENT;
    break;
  }
  MAYBE_UNUSED bool res = librii::hx::configurePixMode(mat, pix_mode);
  assert(res);
}

librii::gx::Comparison compileComparison(librii::rhst::Comparison in) {
  switch (in) {
  case librii::rhst::Comparison::Always:
  default:
    return librii::gx::Comparison::ALWAYS;
  case librii::rhst::Comparison::Equal:
    return librii::gx::Comparison::EQUAL;
  case librii::rhst::Comparison::GEqual:
    return librii::gx::Comparison::GEQUAL;
  case librii::rhst::Comparison::Greater:
    return librii::gx::Comparison::GREATER;
  case librii::rhst::Comparison::LEqual:
    return librii::gx::Comparison::LEQUAL;
  case librii::rhst::Comparison::Less:
    return librii::gx::Comparison::LESS;
  case librii::rhst::Comparison::NEqual:
    return librii::gx::Comparison::NEQUAL;
  case librii::rhst::Comparison::Never:
    return librii::gx::Comparison::NEVER;
  }
}

librii::gx::AlphaOp compileAlphaOp(librii::rhst::AlphaOp in) {
  switch (in) {
  case librii::rhst::AlphaOp::And:
  default:
    return librii::gx::AlphaOp::_and;
  case librii::rhst::AlphaOp::Or:
    return librii::gx::AlphaOp::_or;
  case librii::rhst::AlphaOp::Xnor:
    return librii::gx::AlphaOp::_xnor;
  case librii::rhst::AlphaOp::Xor:
    return librii::gx::AlphaOp::_xor;
  }
}

librii::gx::BlendModeType compileBlendType(librii::rhst::BlendModeType in) {
  switch (in) {
  case librii::rhst::BlendModeType::None:
  default:
    return librii::gx::BlendModeType::none;
  case librii::rhst::BlendModeType::Logic:
    return librii::gx::BlendModeType::logic;
  case librii::rhst::BlendModeType::Blend:
    return librii::gx::BlendModeType::blend;
  case librii::rhst::BlendModeType::Subtract:
    return librii::gx::BlendModeType::subtract;
  }
}

librii::gx::BlendModeFactor
compileBlendFactor(librii::rhst::BlendModeFactor in) {
  switch (in) {
  case librii::rhst::BlendModeFactor::Zero:
    return librii::gx::BlendModeFactor::zero;
  case librii::rhst::BlendModeFactor::One:
    return librii::gx::BlendModeFactor::one;
  case librii::rhst::BlendModeFactor::Src_c:
    return librii::gx::BlendModeFactor::src_c;
  case librii::rhst::BlendModeFactor::Inv_src_c:
    return librii::gx::BlendModeFactor::inv_src_c;
  case librii::rhst::BlendModeFactor::Src_a:
    return librii::gx::BlendModeFactor::src_a;
  case librii::rhst::BlendModeFactor::Inv_src_a:
    return librii::gx::BlendModeFactor::inv_src_a;
  case librii::rhst::BlendModeFactor::Dst_a:
    return librii::gx::BlendModeFactor::dst_a;
  case librii::rhst::BlendModeFactor::Inv_dst_a:
    return librii::gx::BlendModeFactor::inv_dst_a;
  }
}

void compilePESettings(librii::gx::LowLevelGxMaterial& mat,
                       librii::rhst::ProtoMaterial in) {
  // Alpha Test
  compileAlphaMode(mat, librii::rhst::AlphaMode::Opaque); // TEMP
  switch (in.pe.alpha_test) {
  case librii::rhst::AlphaTest::Disabled:
  default:
    mat.alphaCompare = librii::hx::disabled_comparison;
    break;
  case librii::rhst::AlphaTest::Stencil:
    mat.alphaCompare = librii::hx::stencil_comparison;
    break;
  case librii::rhst::AlphaTest::Custom:
    librii::gx::AlphaComparison comparison;
    comparison.compLeft = compileComparison(in.pe.comparison_left);
    comparison.op = compileAlphaOp(in.pe.comparison_op);
    comparison.refLeft = in.pe.comparison_ref_left;
    comparison.refRight = in.pe.comparison_ref_right;
    comparison.compRight = compileComparison(in.pe.comparison_right);
    mat.alphaCompare = comparison;
  }
  // Draw Pass
  mat.xlu = in.pe.xlu;
  // Z Buffer
  mat.earlyZComparison = in.pe.z_early_comparison;
  librii::gx::ZMode z_mode;
  z_mode.compare = in.pe.z_compare;
  z_mode.update = in.pe.z_update;
  z_mode.function = compileComparison(in.pe.z_comparison);
  mat.zMode = z_mode;
  // Dst Alpha
  librii::gx::DstAlpha dst_alpha;
  dst_alpha.enabled = in.pe.dst_alpha_enabled;
  dst_alpha.alpha = in.pe.dst_alpha;
  mat.dstAlpha = dst_alpha;
  // Blend Mode
  librii::gx::BlendMode blend_mode;
  blend_mode.type = compileBlendType(in.pe.blend_type);
  blend_mode.source = compileBlendFactor(in.pe.blend_source);
  blend_mode.dest = compileBlendFactor(in.pe.blend_dest);
  mat.blendMode = blend_mode;
}

void compileMapping(const librii::rhst::ProtoSampler& in,
                    librii::gx::GCMaterialData::TexMatrix &mtx,
                    librii::gx::TexCoordGen& gen) {
  using cmm = libcube::GCMaterialData::CommonMappingMethod;
  using sp = librii::gx::TexGenSrc;

  if (in.mapping == librii::rhst::Mapping::UVMap) {
    mtx.method = cmm::Standard;
    gen.func = librii::gx::TexGenType::Matrix2x4;
    switch (in.uv_map_index) {
    case 0:
      gen.sourceParam = sp::UV0;
      break;
    case 1:
      gen.sourceParam = sp::UV1;
      break;
    case 2:
      gen.sourceParam = sp::UV2;
      break;
    case 3:
      gen.sourceParam = sp::UV3;
      break;
    case 4:
      gen.sourceParam = sp::UV4;
      break;
    case 5:
      gen.sourceParam = sp::UV5;
      break;
    case 6:
      gen.sourceParam = sp::UV6;
      break;
    case 7:
      gen.sourceParam = sp::UV7;
      break;
    default:
      break;
    }
  } else if (in.mapping == librii::rhst::Mapping::EnvMap) {
    mtx.method = cmm::EnvironmentMapping;
    gen.func = librii::gx::TexGenType::Matrix3x4;
    gen.sourceParam = librii::gx::TexGenSrc::Normal;
  } else if (in.mapping == librii::rhst::Mapping::lEnvMap) {
    mtx.method = cmm::EnvironmentLightMapping;
    gen.func = librii::gx::TexGenType::Matrix3x4;
    gen.sourceParam = librii::gx::TexGenSrc::Normal;
    mtx.lightIdx = in.light_index;
  } else if (in.mapping == librii::rhst::Mapping::sEnvMap) {
    mtx.method = cmm::EnvironmentSpecularMapping;
    gen.func = librii::gx::TexGenType::Matrix3x4;
    gen.sourceParam = librii::gx::TexGenSrc::Normal;
    mtx.lightIdx = in.light_index;
  } else if (in.mapping == librii::rhst::Mapping::Projection) {
    mtx.method = cmm::ViewProjectionMapping;
    gen.func = librii::gx::TexGenType::Matrix3x4;
    gen.sourceParam = librii::gx::TexGenSrc::Position;
    mtx.camIdx = in.camera_index;
  }
}

librii::gx::ColorComponent compileSwapTableColor(librii::rhst::Colors in) {
  switch (in) {
  case librii::rhst::Colors::Red:
	return librii::gx::ColorComponent::r;
  case librii::rhst::Colors::Green:
	return librii::gx::ColorComponent::g;
  case librii::rhst::Colors::Blue:
        return librii::gx::ColorComponent::b;
  case librii::rhst::Colors::Alpha:
        return librii::gx::ColorComponent::a;
  }
}

librii::gx::ColorS10 vec4ToColorS10(glm::vec4 in) {
  librii::gx::ColorS10 out;
	out.r = in.r;
	out.g = in.g;
	out.b = in.b;
	out.a = in.a;
    return out;
}

librii::gx::Color vec4ToColor(glm::vec4 in) {
    librii::gx::Color out;
    out.r = in.x;
    out.g = in.y;
    out.b = in.z;
    out.a = in.w;
    return out;
}

void compileMaterial(libcube::IGCMaterial& out,
                     const librii::rhst::ProtoMaterial& in) {
  out.setName(in.name);

  auto& data = out.getMaterialData();

  if (!in.samplers.empty()) {
    for (int i = 0; i < in.samplers.size(); i++) {

	data.samplers.push_back({});
      auto& sampler = data.samplers[i];
      auto sam = in.samplers[i];

      librii::hx::TextureFilter filter;
      filter.minBase = sam.min_filter;
      filter.minMipBase = sam.mip_filter;
      filter.mip = sam.enable_mip;
      sampler.mTexture = sam.texture_name;
      sampler.mWrapU = compileWrap(sam.wrap_u);
      sampler.mWrapV = compileWrap(sam.wrap_v);

      sampler.mLodBias = sam.lod_bias;

      sampler.mMagFilter = sam.mag_filter ? librii::gx::TextureFilter::Linear
                                          : librii::gx::TextureFilter::Near;
      sampler.mMinFilter = lowerTextureFilter(filter);
      
      librii::gx::GCMaterialData::TexMatrix mtx;
      librii::gx::TexCoordGen gen;

	  auto actual_matrix = static_cast<librii::gx::TexMatrix>(
              static_cast<int>(librii::gx::TexMatrix::TexMatrix0) +
              i * 3);
      gen.matrix = actual_matrix;


	  mtx.scale = {sam.scale.x, sam.scale.y};
      mtx.rotate = sam.rotate;
      mtx.translate = {sam.trans.x, sam.trans.y};

	  compileMapping(sam, mtx, gen);

	  data.texMatrices.push_back(mtx);
      data.texGens.push_back(gen);
    }
  }
   else if(!in.texture_name.empty()){
    data.samplers.push_back({});
    auto& sampler = data.samplers[0];
      librii::hx::TextureFilter filter;
      filter.minBase = in.min_filter;
      filter.minMipBase = in.mip_filter;
      filter.mip = in.enable_mip;
      sampler.mTexture = in.texture_name;
      sampler.mWrapU = compileWrap(in.wrap_u);
      sampler.mWrapV = compileWrap(in.wrap_v);

      sampler.mLodBias = in.lod_bias;

      sampler.mMagFilter = in.mag_filter ? librii::gx::TextureFilter::Linear
                                          : librii::gx::TextureFilter::Near;
      sampler.mMinFilter = lowerTextureFilter(filter);;
      data.texGens.push_back({.matrix = librii::gx::TexMatrix::TexMatrix0});
  }
  data.texMatrices.push_back(j3d::Material::TexMatrix{});
	compileCullMode(data, in.show_front, in.show_back);
	if (in.alpha_mode == librii::rhst::AlphaMode::Custom) {
		compilePESettings(data, in);
	} else {
		compileAlphaMode(data, in.alpha_mode);
	}

  if (auto* g3dmat = dynamic_cast<riistudio::g3d::Material*>(&out)) {
    g3dmat->lightSetIndex = in.lightset_index;
    g3dmat->fogIndex = in.fog_index;
  }

  librii::gx::TevStage wip;
  wip.texMap = wip.texCoord = 0;
  wip.rasOrder = librii::gx::ColorSelChanApi::color0a0;
  wip.rasSwap = 0;

  if (in.texture_name.empty() && in.samplers.empty()) {
    wip.colorStage.a = librii::gx::TevColorArg::rasc;
    wip.colorStage.b = librii::gx::TevColorArg::zero;
    wip.colorStage.c = librii::gx::TevColorArg::zero;
    wip.colorStage.d = librii::gx::TevColorArg::zero;

    wip.alphaStage.a = librii::gx::TevAlphaArg::rasa;
    wip.alphaStage.b = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.c = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.d = librii::gx::TevAlphaArg::zero;
  }
  else {
    wip.colorStage.a = librii::gx::TevColorArg::zero;
    wip.colorStage.b = librii::gx::TevColorArg::texc;
    wip.colorStage.c = librii::gx::TevColorArg::rasc;
    wip.colorStage.d = librii::gx::TevColorArg::zero;

    wip.alphaStage.a = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.b = librii::gx::TevAlphaArg::texa;
    wip.alphaStage.c = librii::gx::TevAlphaArg::rasa;
    wip.alphaStage.d = librii::gx::TevAlphaArg::zero;
  } 

  for (int i = 0; i < 4; i++) {
    data.tevColors[i] = vec4ToColorS10(in.tev_colors[i]);
    data.tevKonstColors[i] = vec4ToColor(in.tev_konst_colors[i]);
  }
  data.mStages[0] = wip;

  // Swap Table
  for (int i = 0; i < 4; i++) {
    librii::gx::SwapTableEntry entry;
    librii::rhst::ProtoSwapTableEntry ste = in.swap_table[i];
    entry.r = compileSwapTableColor(ste.r);
    entry.g = compileSwapTableColor(ste.g);
    entry.b = compileSwapTableColor(ste.b);
    entry.a = compileSwapTableColor(ste.a);

	data.mSwapTable[i] = entry;
  }

  librii::gx::ChannelControl ctrl;
  ctrl.enabled = false;
  ctrl.Material = librii::gx::ColorSource::Vertex;
  data.chanData.push_back({});
  data.colorChanControls.push_back(ctrl); // rgb
  data.colorChanControls.push_back(ctrl); // a
}

void compileBillboardMode(libcube::IBoneDelegate& out,
                          const librii::rhst::BillboardMode& billboard_mode) {
  libcube::IBoneDelegate::Billboard bill_mode =
      libcube::IBoneDelegate::Billboard::None;
  switch (billboard_mode) {
  case librii::rhst::BillboardMode::Y_Face:
    bill_mode = libcube::IBoneDelegate::Billboard::Y_Face;
    break;
  case librii::rhst::BillboardMode::Y_Parallel:
    bill_mode = libcube::IBoneDelegate::Billboard::Y_Parallel;
    break;
  case librii::rhst::BillboardMode::Z_Face:
    bill_mode = libcube::IBoneDelegate::Billboard::Z_Face;
    break;
  case librii::rhst::BillboardMode::Z_Parallel:
    bill_mode = libcube::IBoneDelegate::Billboard::Z_Parallel;
    break;
  case librii::rhst::BillboardMode::ZRotate_Face:
    bill_mode = libcube::IBoneDelegate::Billboard::Z_FaceRotate;
    break;
  case librii::rhst::BillboardMode::ZRotate_Parallel:
    bill_mode = libcube::IBoneDelegate::Billboard::Z_ParallelRotate;
    break;
  default:
    bill_mode = libcube::IBoneDelegate::Billboard::None;
    break;
  }

  out.setBillboard(bill_mode);
}

void compileBone(libcube::IBoneDelegate& out, const librii::rhst::Bone& in) {
  out.setName(in.name);
  out.setBoneParent(in.parent);
  for (auto c : in.child) {
    out.addChild(c);
  }
  out.setScale(in.scale);
  out.setRotation(in.rotate);
  out.setTranslation(in.translate);
  out.setAABB({in.min, in.max});
  compileBillboardMode(out, in.billboard_mode);
  for (auto& draw_call : in.draw_calls) {
    out.addDisplay({.matId = static_cast<u32>(draw_call.mat_index),
                    .polyId = static_cast<u32>(draw_call.poly_index),
                    .prio = static_cast<u8>(draw_call.prio)});
  }
}

void compileVert(librii::gx::IndexedVertex& dst,
                 const librii::rhst::Vertex& src, libcube::IndexedPolygon& poly,
                 libcube::Model& mdl) {
  u32 vcd_cursor = 0;

  auto& data = poly.getMeshData();

  for (int i = 0; i < data.mVertexDescriptor.mAttributes.size(); ++i) {
    while ((data.mVertexDescriptor.mBitfield & (1 << vcd_cursor)) == 0) {
      ++vcd_cursor;

      if (vcd_cursor >= 21) {
        assert(!"Invalid");
        return;
      }
    }
    const int cur_attr = vcd_cursor;
    ++vcd_cursor;

    if (cur_attr == 0) {
      dst[librii::gx::VertexAttribute::PositionNormalMatrixIndex] =
          src.matrix_index * 3;
      continue;
    }
    if (cur_attr == 9) {
      dst[librii::gx::VertexAttribute::Position] =
          poly.addPos(mdl, src.position);
      continue;
    }
    if (cur_attr == 10) {
      dst[librii::gx::VertexAttribute::Normal] = poly.addNrm(mdl, src.normal);
      continue;
    }

    if (cur_attr >= 11 && cur_attr <= 12) {
      const int color_index = cur_attr - 11;
      dst[(librii::gx::VertexAttribute)cur_attr] =
          poly.addClr(mdl, color_index, src.colors[color_index]);
      continue;
    }
    if (cur_attr >= 13 && cur_attr <= 20) {
      const int uv_index = cur_attr - 13;
      dst[(librii::gx::VertexAttribute)cur_attr] =
          poly.addUv(mdl, uv_index, src.uvs[uv_index]);
      continue;
    }
  }
}

void compilePrim(librii::gx::IndexedPrimitive& dst,
                 const librii::rhst::Primitive& src,
                 libcube::IndexedPolygon& poly, libcube::Model& model) {
  switch (src.topology) {
  case librii::rhst::Topology::Triangles:
    dst.mType = librii::gx::PrimitiveType::Triangles;
    break;
  case librii::rhst::Topology::TriangleStrip:
    dst.mType = librii::gx::PrimitiveType::TriangleStrip;
    break;
  case librii::rhst::Topology::TriangleFan:
    dst.mType = librii::gx::PrimitiveType::TriangleFan;
    break;
  }

  dst.mVertices.reserve(src.vertices.size());
  for (auto& vert : src.vertices) {
    compileVert(dst.mVertices.emplace_back(), vert, poly, model);
  }
}

[[nodiscard]] Result<void>
compileMatrixPrim(librii::gx::MatrixPrimitive& dst,
                  const librii::rhst::MatrixPrimitive& src, s32 current_matrix,
                  libcube::IndexedPolygon& poly, libcube::Model& model,
                  bool optimize) {
  dst.mCurrentMatrix = current_matrix;
  std::array<s32, 10> empty{
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };
  dst.mDrawMatrixIndices.clear();
  if (src.draw_matrices == empty) {
    dst.mDrawMatrixIndices.push_back(current_matrix);
  } else {
    bool done = false;
    for (s32 d : src.draw_matrices) {
      if (done) {
        EXPECT(d < 0 && "Non-contiguous draw-matrix indices are unsupported");
      }
      if (d < 0) {
        done = true;
        continue;
      }
      dst.mDrawMatrixIndices.push_back(d);
    }
  }

  // Convert to tristrips
  librii::rhst::MatrixPrimitive tmp = src;
  if (optimize) {
    TRY(librii::rhst::StripifyTriangles(tmp));
  }

  for (auto& prim : tmp.primitives) {
    compilePrim(dst.mPrimitives.emplace_back(), prim, poly, model);
  }

  return {};
}

Result<librii::rhst::Mesh> decompileMesh(const libcube::IndexedPolygon& src,
                                         const libcube::Model& mdl) {
  std::optional<s32> current_mtx;
  for (auto& m : src.getMeshData().mMatrixPrimitives) {
    if (!current_mtx) {
      current_mtx = m.mCurrentMatrix;
      continue;
    }
    if (m.mCurrentMatrix != *current_mtx) {
      return std::unexpected("Cannot encode as RHST; meshes are only allowed "
                             "one `current_matrix`. This is only possible with "
                             "a .bmd/.bdl model.");
    }
  }
  if (!current_mtx) {
    return std::unexpected("Mesh is empty");
  }

  librii::rhst::Mesh tmp{.name = src.getName(),
                         .current_matrix = *current_mtx,
                         .vertex_descriptor =
                             src.getMeshData().mVertexDescriptor.mBitfield};
  auto vcd = src.getMeshData().mVertexDescriptor;
  using VA = librii::gx::VertexAttribute;

  u32 allowed = 0;
  allowed |= 1 << static_cast<int>(VA::PositionNormalMatrixIndex);
  allowed |= 1 << static_cast<int>(VA::Position);
  allowed |= 1 << static_cast<int>(VA::Normal);
  for (size_t i = 0; i < 2; ++i) {
    allowed |= 1 << static_cast<int>(librii::gx::ColorN(i));
  }
  for (size_t i = 0; i < 8; ++i) {
    allowed |= 1 << static_cast<int>(librii::gx::TexCoordN(i));
  }
  u32 unsupported = vcd.mBitfield & ~allowed;
  if (unsupported != 0) {
    std::vector<std::string_view> bad;
    for (size_t i = 0; i < static_cast<size_t>(VA::Max); ++i) {
      if (unsupported & (1 << i)) {
        bad.push_back(magic_enum::enum_name(static_cast<VA>(i)));
      }
    }
    return std::unexpected(std::format("Mesh has unsupported attributes: {}",
                                       rsl::join(bad, ", ")));
  }

  if (!vcd[librii::gx::VertexAttribute::Position]) {
    return std::unexpected("Position attribute is required");
  }
  libcube::PolyIndexer indexer(src, mdl);
  for (auto& x : src.getMeshData().mMatrixPrimitives) {
    librii::rhst::MatrixPrimitive& mp = tmp.matrix_primitives.emplace_back();
    for (size_t i = 0; i < x.mDrawMatrixIndices.size(); ++i) {
      EXPECT(i < 10 && "Mesh has too many draw matrices");
      mp.draw_matrices[i] = x.mDrawMatrixIndices[i];
    }
    for (auto& y : x.mPrimitives) {
      auto& p = mp.primitives.emplace_back();
      switch (y.mType) {
      case librii::gx::PrimitiveType::Triangles:
        p.topology = librii::rhst::Topology::Triangles;
        break;
      case librii::gx::PrimitiveType::TriangleStrip:
        p.topology = librii::rhst::Topology::TriangleStrip;
        break;
      case librii::gx::PrimitiveType::TriangleFan:
        p.topology = librii::rhst::Topology::TriangleFan;
        break;
      default:
        return std::unexpected(
            std::format("Unexpected topology {}. Expected Tris/Strips/Fans.",
                        magic_enum::enum_name(y.mType)));
      }
      for (auto& z : y.mVertices) {
        auto& v = p.vertices.emplace_back();

        v.position = TRY(indexer.positions[z[VA::Position]]);
        if (vcd[VA::PositionNormalMatrixIndex]) {
          auto cand = z[VA::PositionNormalMatrixIndex];
          EXPECT(cand % 3 == 0);
          v.matrix_index = cand / 3;
        }
        if (librii::rhst::hasNormal(vcd.mBitfield)) {
          v.normal = TRY(indexer.normals[z[VA::Normal]]);
        }
        for (size_t i = 0; i < 2; ++i) {
          if (librii::rhst::hasColor(vcd.mBitfield, i)) {
            auto clr = TRY(indexer.colors[i][z[librii::gx::ColorN(i)]]);
            auto fc = static_cast<librii::gx::ColorF32>(clr);
            v.colors[i] = {fc.r, fc.g, fc.b, fc.a};
          }
        }
        for (size_t i = 0; i < 8; ++i) {
          if (librii::rhst::hasTexCoord(vcd.mBitfield, i)) {
            auto uv = TRY(indexer.uvs[i][z[librii::gx::TexCoordN(i)]]);
            v.uvs[i] = uv;
          }
        }
      }
    }
  }
  return tmp;
}

Result<void> compileMesh(libcube::IndexedPolygon& dst,
                         const librii::rhst::Mesh& src, libcube::Model& model,
                         bool optimize, bool reinit_bufs) {
  dst.setName(src.name);

  // No skinning/BB
  dst.init(false, nullptr);
  auto& data = dst.getMeshData();
  dst.setCurMtx(src.current_matrix);
  data.mMatrixPrimitives.clear();

#if 0
  fprintf(stderr, "Compiling %s \n", src.name.c_str());
#endif
  for (int i = 0; i < static_cast<int>(librii::gx::VertexAttribute::Max); ++i) {
    if ((src.vertex_descriptor & (1 << i)) == 0)
      continue;

    // PNMTXIDX is direct u8
    auto fmt = i == 0 ? librii::gx::VertexAttributeType::Direct
                      : librii::gx::VertexAttributeType::Short;
    data.mVertexDescriptor.mAttributes.emplace(
        static_cast<librii::gx::VertexAttribute>(i), fmt);
  }

  data.mVertexDescriptor.calcVertexDescriptorFromAttributeList();

  EXPECT(data.mVertexDescriptor.mBitfield == src.vertex_descriptor);
  if (reinit_bufs) {
    dst.initBufsFromVcd(model);
    dst.setCurMtx(src.current_matrix);
  }

  for (auto& matrix_prim : src.matrix_primitives) {
    TRY(compileMatrixPrim(data.mMatrixPrimitives.emplace_back(), matrix_prim,
                          src.current_matrix, dst, model, optimize));
  }

  for (auto& [attr, format] : data.mVertexDescriptor.mAttributes) {
    if (format != librii::gx::VertexAttributeType::Short) {
      continue;
    }
    u16 max = 0;
    for (auto& mp : data.mMatrixPrimitives) {
      for (auto& p : mp.mPrimitives) {
        for (auto& v : p.mVertices) {
          // TODO: Checked
          u16 i = v[attr];
          if (i > max) {
            max = i;
          }
        }
      }
    }
    if (max <= std::numeric_limits<u8>::max()) {
      format = librii::gx::VertexAttributeType::Byte;
    }
  }

  return {};
}

static inline std::string getFileShort(const std::string& path) {
  auto tmp = path.substr(path.rfind("\\") + 1);
  // tmp = tmp.substr(0, tmp.rfind("."));
  return tmp;
}
Result<void> importTextureImpl(libcube::Texture& data, std::span<u8> image,
                               std::vector<u8>& scratch, int num_mip, int width,
                               int height, int source_w, int source_h,
                               librii::gx::TextureFormat fmt,
                               librii::image::ResizingAlgorithm resize) {
  data.setTextureFormat(fmt);
  data.setWidth(width);
  data.setHeight(height);
  data.setMipmapCount(num_mip);
  data.setLod(false, 0.0f, static_cast<f32>(data.getImageCount()));
  data.resizeData();
  if (num_mip == 0) {
    std::vector<u8> scratch(4 * width * height);
    librii::image::resize(scratch, width, height, image, source_w, source_h,
                          resize);
    TRY(data.encode(scratch));
  } else {
    rsl::trace("Width: {}, Height: {}.", width, height);
    u32 size = 0;
    for (int i = 0; i <= num_mip; ++i) {
      size += (width >> i) * (height >> i) * 4;
      rsl::trace("Image {}: {}, {}. -> Total Size: {}", i, width >> i,
                 height >> i, size);
    }
    scratch.resize(size);

    u32 slide = 0;
    for (int i = 0; i <= num_mip; ++i) {
      librii::image::resize(std::span(scratch).subspan(slide), width >> i,
                            height >> i, image, source_w, source_h, resize);
      slide += (width >> i) * (height >> i) * 4;
    }

    TRY(data.encode(scratch));
  }
  return {};
}
Result<void> importTexture(libcube::Texture& data, std::span<u8> image,
                           std::vector<u8>& scratch, bool mip_gen, int min_dim,
                           int max_mip, int width, int height, int channels) {
  if (image.empty()) {
    return std::unexpected(
        "STB failed to parse image. Unsupported file format?");
  }
  if (width > 1024) {
    return std::unexpected(
        std::format("Width {} exceeds maximum of 1024", width));
  }
  if (height > 1024) {
    return std::unexpected(
        std::format("Height {} exceeds maximum of 1024", height));
  }
  if (!is_power_of_2(width)) {
    return std::unexpected(std::format("Width {} is not a power of 2.", width));
  }
  if (!is_power_of_2(height)) {
    return std::unexpected(
        std::format("Height {} is not a power of 2.", height));
  }

  int num_mip = 0;
  if (mip_gen && is_power_of_2(width) && is_power_of_2(height)) {
    while ((num_mip + 1) < max_mip && (width >> (num_mip + 1)) >= min_dim &&
           (height >> (num_mip + 1)) >= min_dim)
      ++num_mip;
  }

  return importTextureImpl(data, image, scratch, num_mip, width, height, width,
                           height, librii::gx::TextureFormat::CMPR);
}

Result<void> importTextureFromMemory(libcube::Texture& data,
                                     std::span<const u8> span,
                                     std::vector<u8>& scratch, bool mip_gen,
                                     int min_dim, int max_mip) {
  auto image = TRY(rsl::stb::load_from_memory(span));
  return importTexture(data, image.data, scratch, mip_gen, min_dim, max_mip,
                       image.width, image.height, image.channels);
}
Result<void> importTextureFromFile(libcube::Texture& data,
                                   std::string_view path,
                                   std::vector<u8>& scratch, bool mip_gen,
                                   int min_dim, int max_mip) {
  if (path.ends_with(".tex0")) {
    auto obuf = ReadFile(path);
    if (!obuf) {
      return std::unexpected("Failed to read file");
    }
    auto tex = TRY(librii::crate::ReadTEX0(*obuf));
    data.setTextureFormat(tex.format);
    data.setWidth(tex.width);
    data.setHeight(tex.height);
    data.setImageCount(tex.number_of_images);
    data.setLod(false, 0.0f, static_cast<f32>(data.getImageCount()));
    data.resizeData();
    auto span = data.getData();
    EXPECT(span.size() == tex.data.size());
    memcpy(span.data(), tex.data.data(), span.size());
    return {};
  }
  auto image = TRY(rsl::stb::load(path));
  return importTexture(data, image.data, scratch, mip_gen, min_dim, max_mip,
                       image.width, image.height, image.channels);
}

void import_texture(std::string tex, libcube::Texture* pdata,
                    std::filesystem::path file_path,
                    std::optional<MipGen> mips) {
  libcube::Texture& data = *pdata;
  std::vector<u8> scratch;
  bool mip_gen = mips.has_value();
  u32 min_dim = mips ? mips->min_dim : 0;
  u32 max_mip = mips ? mips->max_mip : 0;

  std::vector<std::filesystem::path> search_paths;
  search_paths.push_back(file_path);
  search_paths.push_back(file_path.parent_path() / (tex + ".tex0"));
  search_paths.push_back(file_path.parent_path() / "textures" /
                         (tex + ".tex0"));
  search_paths.push_back(file_path.parent_path() / (tex + ".png"));
  search_paths.push_back(file_path.parent_path() / "textures" / (tex + ".png"));

  for (const auto& path : search_paths) {
    if (importTextureFromFile(data, path.string().c_str(), scratch, mip_gen,
                              min_dim, max_mip)) {
      return;
    }
  }
  rsl::error("Cannot find texture {}", tex.c_str());
  // 32x32 (32bpp)
  const auto dummy_width = 32;
  const auto dummy_height = 32;
  data.setWidth(dummy_width);
  data.setHeight(dummy_height);
  scratch.resize(dummy_width * dummy_height * 4);
  // Make a basic checkerboard
  librii::image::generateCheckerboard(scratch, dummy_width, dummy_height);
  data.setMipmapCount(0);
  data.setTextureFormat(librii::gx::TextureFormat::CMPR);
  data.encode(scratch);
  // unresolved.emplace(i, tex);
}

bool CompileRHST(librii::rhst::SceneTree& rhst, libcube::Scene& scene,
                 std::string path,
                 std::function<void(std::string, std::string)> info,
                 std::function<void(std::string_view, float)> progress,
                 std::optional<MipGen> mips, bool tristrip, bool verbose) {
  std::set<std::string> textures_needed;

  for (auto& mat : rhst.materials) {
    if (mat.samplers.empty()) {
      if (!mat.texture_name.empty()) {
        textures_needed.emplace(mat.texture_name);
      }
    } else {
      for (auto& sam : mat.samplers) {
        if (!sam.texture_name.empty()) {
          textures_needed.emplace(sam.texture_name);
        }
      }
    }
    
  }

  libcube::Model& mdl = scene.getModels().add();

  for (auto& mat : rhst.materials) {
    compileMaterial(mdl.getMaterials().add(), mat);
  }
  librii::math::AABB aabb{{FLT_MAX, FLT_MAX, FLT_MAX},
                          {FLT_MIN, FLT_MIN, FLT_MIN}};
  for (auto& bone : rhst.bones) {
    compileBone(mdl.getBones().add(), bone);
    aabb.expandBound(librii::math::AABB{bone.min, bone.max});
  }
  if (auto* g = dynamic_cast<riistudio::g3d::Model*>(&mdl)) {
    g->aabb = aabb;
    g->mName = rhst.name;
  }

  for (auto& tex : textures_needed) {
    rsl::info("Importing texture: {}", tex.c_str());

    auto& data = scene.getTextures().add();
    data.setName(getFileShort(tex));
  }
  // Favor PNG, and the current directory
  auto file_path = std::filesystem::path(path);

  std::vector<std::future<void>> futures;

  for (int i = 0; i < scene.getTextures().size(); ++i) {
    libcube::Texture* data = &scene.getTextures()[i];

    futures.push_back(std::async(std::launch::async, [=] {
      import_texture(data->getName(), data, file_path, mips);
    }));
  }

  // Optimize meshes
  if (tristrip) {
    progress(std::format("Optimizing meshes {}/{}", 0, 0), 0.0f);
    std::atomic<int> so_far = 0;
    int total = rhst.meshes.size();
    progress(std::format("Optimizing meshes ({} / {})", 0, total), 0.0f);

    rsl::Timer timer;
    for (auto& mesh : rhst.meshes) {
      auto task = [&](librii::rhst::Mesh* mesh) {
        assert(mesh != nullptr);
        int i = 0;
        for (auto& mp : mesh->matrix_primitives) {
          auto ok = librii::rhst::StripifyTriangles(
              mp, std::nullopt,
              mesh->matrix_primitives.size() > 1
                  ? std::format("{}::{}", mesh->name, i)
                  : mesh->name,
              verbose);
          ++i;
          if (!ok) {
            rsl::error("Error: Failed to stripify mesh {}. {}", mesh->name,
                       ok.error());
          }
        }
        ++so_far;
        int x = static_cast<int>(so_far);
        progress(std::format("Optimizing meshes ({} / {})", x, total),
                 static_cast<float>(x) / static_cast<float>(total));
      };
      futures.push_back(std::async(std::launch::async, std::bind(task, &mesh)));
    }

    futures.clear();
    rsl::error("Elapsed stripping time (multicore) (or texture import time if "
               "greater): {}ms",
               timer.elapsed());
  }

  progress(std::format("Compiling meshes {}/{}", 0, rhst.meshes.size()), 0.0f);
  for (auto&& [i, mesh] : rsl::enumerate(rhst.meshes)) {
    progress(std::format("Compiling meshes {}/{}", i, rhst.meshes.size()),
             static_cast<float>(i) / static_cast<float>(rhst.meshes.size()));
    // Already optimized (and in parallel)
    auto ok = compileMesh(mdl.getMeshes().add(), mesh, mdl, false);
    if (!ok) {
      rsl::error("ERROR: Failed to compile mesh: {}", ok.error().c_str());
      continue;
    }
  }

  for (auto& weight : rhst.weights) {
    auto& bweightgroup = mdl.mDrawMatrices.emplace_back();

    for (auto& influence : weight.weights) {
      auto& bweight = bweightgroup.mWeights.emplace_back();

      bweight.boneId = influence.bone_index;
      bweight.weight = static_cast<float>(influence.influence) / 100.0f;
    }
  }

  // Now that all textures are loaded, correct sampler settings
  for (auto& mat : mdl.getMaterials()) {
    for (auto& sampler : mat.getMaterialData().samplers) {
      const auto* tex = mat.getTexture(scene, sampler.mTexture);
      if (tex == nullptr) {
        continue;
      }
      // Correction: Wrapping hardware only operates on power-of-two inputs.
      const auto clamp = librii::gx::TextureWrapMode::Clamp;
      if (!is_power_of_2(tex->getWidth()) && sampler.mWrapU != clamp) {
        info(mat.getName(),
             "Texture is not a power of two in the U direction, Repeat/Mirror "
             "wrapping is unsupported.");
        sampler.mWrapU = clamp;
      }
      if (!is_power_of_2(tex->getHeight()) && sampler.mWrapV != clamp) {
        info(mat.getName(),
             "Texture is not a power of two in the V direction, Repeat/Mirror "
             "wrapping is unsupported.");
        sampler.mWrapV = clamp;
      }
      // Correction: Intermediate format doesn't specify texture filtering.
    }
  }

  progress("Applying material presets", 0.0f);
  // Handle material presets
  if (auto* gmdl = dynamic_cast<g3d::Model*>(&mdl); gmdl != nullptr) {
    for (auto& mat : rhst.materials) {
      if (mat.preset_path_mdl0mat.empty()) {
        continue;
      }
      auto* gmat = gmdl->getMaterials().findByName(mat.name);
      if (mat.preset_path_mdl0mat.ends_with(".rspreset")) {
        rsl::debug("Applying .rspreset preset to material {} from path {}",
                   mat.name, mat.preset_path_mdl0mat);
        auto file = rsl::ReadOneFile(mat.preset_path_mdl0mat);
        if (!file) {
          rsl::debug("...Error: {}", file.error());
          continue;
        }
        auto ok = ApplyRSPresetToMaterial(*gmat, file->data);
        if (!ok) {
          rsl::debug("...Error: {}", ok.error());
        } else {
          rsl::debug("...Sucess");
        }
      } else {
        rsl::debug("Applying .mdl0mat preset to material {} from path {}",
                   mat.name, mat.preset_path_mdl0mat);
        auto ok = ApplyCratePresetToMaterial(*gmat, mat.preset_path_mdl0mat);
        if (!ok) {
          rsl::debug("...Error: {}", ok.error());
        } else {
          rsl::debug("...Sucess");
        }
      }
    }

    // Remove unused textures
    // NOTE: This *must* be done on concrete types, as copy ctors are invalid
    // for interface types.
    auto* gscn = dynamic_cast<g3d::Collection*>(&scene);
    assert(gscn);
    std::unordered_set<std::string> usedTextures;
    for (auto& mat : mdl.getMaterials()) {
      for (auto& sampler : mat.getMaterialData().samplers) {
        usedTextures.insert(sampler.mTexture);
      }
    }
    std::vector<std::string> unused_names;
    size_t n = std::distance(
        gscn->getTextures().begin(),
        std::remove_if(gscn->getTextures().begin(), gscn->getTextures().end(),
                       [&](auto&& tex) {
                         bool unused = !usedTextures.contains(tex.name);
                         if (unused) {
                           unused_names.push_back(
                               std::format("\"{}\"", tex.name));
                         }
                         return unused;
                       }));

    rsl::info("Removing {} unreferenced textures: {}.",
              gscn->getTextures().size() - n, rsl::join(unused_names, ","));
    gscn->getTextures().resize(n);
  }

  return true;
}

} // namespace riistudio::rhst
