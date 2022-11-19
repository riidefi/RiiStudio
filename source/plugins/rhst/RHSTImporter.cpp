#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Plugins.hpp>
#include <filesystem>
#include <librii/hx/CullMode.hpp>
#include <librii/hx/PixMode.hpp>
#include <librii/rhst/RHST.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <plugins/g3d/collection.hpp>
#include <plugins/gc/Export/Scene.hpp>
#include <plugins/j3d/Material.hpp>
#include <plugins/j3d/Scene.hpp>
#include <set>
#include <stb_image.h>
#include <string>
#include <vendor/thread_pool.hpp>

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
  }
  MAYBE_UNUSED bool res = librii::hx::configurePixMode(mat, pix_mode);
  assert(res);
}

void compileMaterial(libcube::IGCMaterial& out,
                     const librii::rhst::Material& in) {
  out.setName(in.name);

  auto& data = out.getMaterialData();

  if (!in.texture_name.empty()) {
    data.samplers.push_back({});
    auto& sampler = data.samplers[0];
    sampler.mTexture = in.texture_name;
    sampler.mWrapU = compileWrap(in.wrap_u);
    sampler.mWrapV = compileWrap(in.wrap_v);

    // NOTE: As textures are loaded on separate threads, the min filter will be
    // corrected then.
    sampler.mMinFilter = librii::gx::TextureFilter::Linear;

    compileCullMode(data, in.show_front, in.show_back);
    // TODO: lightset, fog indices
    compileAlphaMode(data, in.alpha_mode);

    data.texMatrices.push_back(j3d::Material::TexMatrix{});
    data.texGens.push_back({.matrix = librii::gx::TexMatrix::TexMatrix0});
  }

  librii::gx::TevStage wip;
  wip.texMap = wip.texCoord = 0;
  wip.rasOrder = librii::gx::ColorSelChanApi::color0a0;
  wip.rasSwap = 0;

  if (!in.texture_name.empty()) {
    wip.colorStage.a = librii::gx::TevColorArg::zero;
    wip.colorStage.b = librii::gx::TevColorArg::texc;
    wip.colorStage.c = librii::gx::TevColorArg::rasc;
    wip.colorStage.d = librii::gx::TevColorArg::zero;

    wip.alphaStage.a = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.b = librii::gx::TevAlphaArg::texa;
    wip.alphaStage.c = librii::gx::TevAlphaArg::rasa;
    wip.alphaStage.d = librii::gx::TevAlphaArg::zero;
  } else {
    wip.colorStage.a = librii::gx::TevColorArg::rasc;
    wip.colorStage.b = librii::gx::TevColorArg::zero;
    wip.colorStage.c = librii::gx::TevColorArg::zero;
    wip.colorStage.d = librii::gx::TevColorArg::zero;

    wip.alphaStage.a = librii::gx::TevAlphaArg::rasa;
    wip.alphaStage.b = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.c = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.d = librii::gx::TevAlphaArg::zero;
  }

  data.tevColors[0] = {0xaa, 0xbb, 0xcc, 0xff};
  data.mStages[0] = wip;

  librii::gx::ChannelControl ctrl;
  ctrl.enabled = false;
  ctrl.Material = librii::gx::ColorSource::Vertex;
  data.chanData.push_back({});
  data.colorChanControls.push_back(ctrl); // rgb
  data.colorChanControls.push_back(ctrl); // a
}

void compileBone(libcube::IBoneDelegate& out, const librii::rhst::Bone& in) {
  out.setName(in.name);

  out.setBoneParent(in.parent);
  // TODO: Children
  out.setScale(in.scale);
  out.setRotation(in.rotate);
  out.setTranslation(in.translate);
  out.setAABB({in.min, in.max});
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

void compileMatrixPrim(librii::gx::MatrixPrimitive& dst,
                       const librii::rhst::MatrixPrimitive& src,
                       s32 current_matrix, libcube::IndexedPolygon& poly,
                       libcube::Model& model) {
  dst.mCurrentMatrix = current_matrix;
  dst.mDrawMatrixIndices.push_back(current_matrix);
  // TODO: Support these
  // std::copy(src.draw_matrices.begin(), src.draw_matrices.end(),
  //           std::back_inserter(dst.mDrawMatrixIndices));

  for (auto& prim : src.primitives) {
    compilePrim(dst.mPrimitives.emplace_back(), prim, poly, model);
  }
}

void compileMesh(libcube::IndexedPolygon& dst, const librii::rhst::Mesh& src,
                 int id, libcube::Model& model) {
  dst.setName(src.name);
  dst.setId(id);

  // No skinning/BB
  dst.init(false, nullptr);
  auto& data = dst.getMeshData();

  for (int i = 0; i < static_cast<int>(librii::gx::VertexAttribute::Max); ++i) {
    if ((src.vertex_descriptor & (1 << i)) == 0)
      continue;

    data.mVertexDescriptor.mAttributes.emplace(
        static_cast<librii::gx::VertexAttribute>(i),
        librii::gx::VertexAttributeType::Short);
  }

  data.mVertexDescriptor.calcVertexDescriptorFromAttributeList();

  assert(data.mVertexDescriptor.mBitfield == src.vertex_descriptor);
  dst.initBufsFromVcd(model);

  for (auto& matrix_prim : src.matrix_primitives) {
    compileMatrixPrim(data.mMatrixPrimitives.emplace_back(), matrix_prim, 0,
                      dst, model);
  }
}

struct RHSTReader {
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const;
  void read(kpi::IOTransaction& transaction);
};

kpi::Register<RHSTReader, kpi::Reader> RHSTInstaller;

std::string RHSTReader::canRead(const std::string& file,
                                oishii::BinaryReader& reader) const {
  if (reader.read<u32, oishii::EndianSelect::Big>() != 'RHST')
    return "";

  if (reader.read<u32, oishii::EndianSelect::Little>() != 1)
    return "";

  if (rebuild_dest.ends_with(".brres"))
    return typeid(g3d::Collection).name();

  if (rebuild_dest.ends_with(".bmd"))
    return typeid(j3d::Collection).name();

  return typeid(lib3d::Scene).name();
}

static inline std::string getFileShort(const std::string& path) {
  auto tmp = path.substr(path.rfind("\\") + 1);
  // tmp = tmp.substr(0, tmp.rfind("."));
  return tmp;
}

static bool importTexture(libcube::Texture& data, u8* image,
                          std::vector<u8>& scratch, bool mip_gen, int min_dim,
                          int max_mip, int width, int height, int channels) {
  if (!image) {
    data.setWidth(0);
    data.setHeight(0);
    return false;
  }
  assert(image);

  int num_mip = 0;
  if (mip_gen && is_power_of_2(width) && is_power_of_2(height)) {
    while ((num_mip + 1) < max_mip && (width >> (num_mip + 1)) >= min_dim &&
           (height >> (num_mip + 1)) >= min_dim)
      ++num_mip;
  }
  data.setTextureFormat(librii::gx::TextureFormat::CMPR);
  data.setWidth(width);
  data.setHeight(height);
  data.setMipmapCount(num_mip);
  data.resizeData();
  if (num_mip == 0) {
    data.encode(image);
  } else {
    printf("Width: %u, Height: %u.\n", (unsigned)width, (unsigned)height);
    u32 size = 0;
    for (int i = 0; i <= num_mip; ++i) {
      size += (width >> i) * (height >> i) * 4;
      printf("Image %i: %u, %u. -> Total Size: %u\n", i, (unsigned)(width >> i),
             (unsigned)(height >> i), size);
    }
    scratch.resize(size);

    u32 slide = 0;
    for (int i = 0; i <= num_mip; ++i) {
      librii::image::resize(scratch.data() + slide, width >> i, height >> i,
                            image, width, height, librii::image::Lanczos);
      slide += (width >> i) * (height >> i) * 4;
    }

    data.encode(scratch.data());
  }
  stbi_image_free(image);
  return true;
}
static bool importTexture(libcube::Texture& data, const char* path,
                          std::vector<u8>& scratch, bool mip_gen, int min_dim,
                          int max_mip) {
  int width, height, channels;
  u8* image = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
  return importTexture(data, image, scratch, mip_gen, min_dim, max_mip, width,
                       height, channels);
}

void import_texture(std::string tex, libcube::Texture* pdata,
                    std::filesystem::path file_path) {
  libcube::Texture& data = *pdata;
  std::vector<u8> scratch;
  bool mip_gen = true;
  u32 min_dim = 64;
  u32 max_mip = 3;

  const auto alt_path =
      (file_path.parent_path() / "textures" / (tex + ".png")).string();

  if (!importTexture(data, tex.c_str(), scratch, mip_gen, min_dim, max_mip)) {
    if (!importTexture(data, alt_path.c_str(), scratch, mip_gen, min_dim,
                       max_mip)) {
      printf("Cannot find texture %s\n", tex.c_str());
      // unresolved.emplace(i, tex);
    }
  }
};

void RHSTReader::read(kpi::IOTransaction& transaction) {
  std::string error_msg;
  auto result = librii::rhst::ReadSceneTree(transaction.data, error_msg);

  if (!result.has_value()) {
    transaction.state = kpi::TransactionState::Failure;
    return;
  }
  int hw_threads = std::thread::hardware_concurrency();
  // Account for the main thread
  if (hw_threads > 1)
    --hw_threads;
  thread_pool pool(hw_threads);

  std::set<std::string> textures_needed;

  for (auto& mat : result->materials) {
    if (!mat.texture_name.empty())
      textures_needed.emplace(mat.texture_name);
  }

  libcube::Scene* pnode = dynamic_cast<libcube::Scene*>(&transaction.node);
  assert(pnode != nullptr);
  libcube::Scene& scene = *pnode;

  libcube::Model& mdl = scene.getModels().add();

  for (auto& mat : result->materials) {
    compileMaterial(mdl.getMaterials().add(), mat);
  }
  for (auto& bone : result->bones) {
    compileBone(mdl.getBones().add(), bone);
  }

  for (auto& tex : textures_needed) {
    printf("Importing texture: %s\n", tex.c_str());

    auto& data = scene.getTextures().add();
    data.setName(getFileShort(tex));
  }
  // Favor PNG, and the current directory
  auto file_path =
      std::filesystem::path(transaction.data.getProvider()->getFilePath());

  std::vector<std::future<bool>> futures;

  for (int i = 0; i < scene.getTextures().size(); ++i) {
    libcube::Texture* data = &scene.getTextures()[i];

    auto task = std::bind(&import_texture, data->getName(), data, file_path);
    futures.push_back(pool.submit(task));
  }

  int i = 0;
  for (auto& mesh : result->meshes) {
    compileMesh(mdl.getMeshes().add(), mesh, i++, mdl);
  }

  for (auto& weight : result->weights) {
    auto& bweightgroup = mdl.mDrawMatrices.emplace_back();

    for (auto& influence : weight.weights) {
      auto& bweight = bweightgroup.mWeights.emplace_back();

      bweight.boneId = influence.bone_index;
      bweight.weight = static_cast<float>(influence.influence) / 100.0f;
    }
  }

  for (auto& f : futures) {
    f.get();
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
        transaction.callback(
            kpi::IOMessageClass::Information, mat.getName(),
            "Texture is not a power of two in the U direction, Repeat/Mirror "
            "wrapping is unsupported.");
        sampler.mWrapU = clamp;
      }
      if (!is_power_of_2(tex->getHeight()) && sampler.mWrapV != clamp) {
        transaction.callback(
            kpi::IOMessageClass::Information, mat.getName(),
            "Texture is not a power of two in the V direction, Repeat/Mirror "
            "wrapping is unsupported.");
        sampler.mWrapV = clamp;
      }
      // Correction: Intermediate format doesn't specify texture filtering.
      if (tex->getMipmapCount() > 0) {
        sampler.mMinFilter = librii::gx::TextureFilter::lin_mip_lin;
      }
    }
  }

  // Handle material presets
  if (auto* gmdl = dynamic_cast<g3d::Model*>(&mdl); gmdl != nullptr) {
    for (auto& mat : result->materials) {
      if (mat.preset_path_mdl0mat.empty()) {
        continue;
      }
      auto* gmat = gmdl->getMaterials().findByName(mat.name);
      DebugReport("Applying .mdl0mat preset to material %s from path %s\n",
                  mat.name.c_str(), mat.preset_path_mdl0mat.c_str());
      auto err = ApplyCratePresetToMaterial(*gmat, mat.preset_path_mdl0mat);
      if (err.size()) {
        DebugReport("...Error: %s\n", err.c_str());
      } else {
        DebugReport("...Sucess\n");
      }
    }
  }

  transaction.state = kpi::TransactionState::Complete;
}

} // namespace riistudio::rhst
