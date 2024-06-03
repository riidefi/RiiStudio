#include <core/common.h>
#include <expected>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <rsl/Try.hpp>
#include <span>

#include <librii/g3d/data/Archive.hpp>
#include <librii/g3d/data/BoneData.hpp>
#include <librii/g3d/data/PolygonData.hpp>
#include <librii/g3d/data/TextureData.hpp>
#include <librii/g3d/data/VertexData.hpp>
#include <librii/g3d/io/ArchiveIO.hpp>

template <typename T> using Expected = std::expected<T, std::string>;

struct JsonReadCtx {
  nlohmann::json j;
  std::span<const std::span<const u8>> buffers;

  template <typename T> Expected<T> get(const std::string& name) const {
    auto it = j.find(name);
    if (it == j.end()) {
      return std::unexpected(name + " not found");
    }
    return it->template get<T>();
  }

  Expected<glm::vec2> getVec2(const std::string& name) const {
    auto it = j.find(name);
    if (it == j.end()) {
      return std::unexpected(name + " not found");
    }
    auto x = (*it)[0].template get<f32>();
    auto y = (*it)[1].template get<f32>();
    return glm::vec2(x, y);
  }

  Expected<glm::vec3> getVec3(const std::string& name) const {
    auto it = j.find(name);
    if (it == j.end()) {
      return std::unexpected(name + " not found");
    }
    auto x = (*it)[0].template get<f32>();
    auto y = (*it)[1].template get<f32>();
    auto z = (*it)[2].template get<f32>();
    return glm::vec3(x, y, z);
  }

  std::optional<glm::vec2> getVec2(int& i) const {
    auto j_ = j[i++];
    auto x = (j_)[0].template get<f32>();
    auto y = (j_)[1].template get<f32>();
    return glm::vec2(x, y);
  }
  std::optional<glm::vec3> getVec3(int& i) const {
    auto j_ = j[i++];
    auto x = (j_)[0].template get<f32>();
    auto y = (j_)[1].template get<f32>();
    auto z = (j_)[2].template get<f32>();
    return glm::vec3(x, y, z);
  }
  std::optional<glm::vec4> getVec4(int& i) const {
    auto j_ = j[i++];
    auto x = (j_)[0].template get<f32>();
    auto y = (j_)[1].template get<f32>();
    auto z = (j_)[2].template get<f32>();
    auto w = (j_)[3].template get<f32>();
    return glm::vec4(x, y, z, w);
  }

  template <typename T> Expected<std::span<const T>> buffer(int index) const {
    if (index > static_cast<int>(buffers.size()) || index < 0) {
      return std::unexpected("Invalid buffer index");
    }
    return buffers[index];
  }
};

struct JsonWriteCtx {
  std::vector<std::vector<u8>> buffers;

  int save_buffer_with_move(std::vector<u8>&& buf) {
    buffers.emplace_back(std::move(buf));
    return static_cast<int>(buffers.size()) - 1;
  }
  template <typename T> int save_buffer_with_copy(std::span<const T> buf) {
    const u8* begin = reinterpret_cast<const u8*>(buf.data());
    std::vector<u8> tmp(begin, begin + buf.size() * sizeof(T));
    return save_buffer_with_move(std::move(tmp));
  }
};

Expected<librii::g3d::TextureData> FromJson(const JsonReadCtx& ctx) {
  librii::g3d::TextureData tmp;
  tmp.name = TRY(ctx.get<std::string>("name"));
  tmp.format =
      static_cast<librii::gx::TextureFormat>(TRY(ctx.get<int>("format")));
  tmp.width = TRY(ctx.get<u32>("width"));
  tmp.height = TRY(ctx.get<u32>("height"));
  tmp.number_of_images = TRY(ctx.get<u32>("number_of_images"));
  tmp.custom_lod = false;
  tmp.minLod = TRY(ctx.get<f32>("minLod"));
  tmp.maxLod = TRY(ctx.get<f32>("maxLod"));

  tmp.sourcePath = TRY(ctx.get<std::string>("sourcePath"));

  auto buf = TRY(ctx.buffer<u8>(TRY(ctx.get<int>("dataBufferId"))));
  tmp.data.assign(buf.begin(), buf.end());
  return tmp;
}
void WriteJson(JsonWriteCtx& ctx, nlohmann::json& j,
               const librii::g3d::TextureData& tex) {
  j["name"] = tex.name;
  j["format"] = static_cast<int>(tex.format);
  j["width"] = tex.width;
  j["height"] = tex.height;
  j["number_of_images"] = tex.number_of_images;
  j["minLod"] = tex.minLod;
  j["maxLod"] = tex.maxLod;
  j["sourcePath"] = tex.sourcePath;
  j["dataBufferId"] = ctx.save_buffer_with_copy<u8>(tex.data);
}

template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
void WriteJson(
    JsonWriteCtx& c, nlohmann::json& j,
    const librii::g3d::GenericBuffer<T, HasMinimum, HasDivisor, kind>& b) {
  j["name"] = b.mName;
  j["id"] = b.mId;
  if constexpr (kind == librii::gx::VertexBufferKind::position) {
    j["q_comp"] = static_cast<int>(b.mQuantize.mComp.position);
  }
  if constexpr (kind == librii::gx::VertexBufferKind::normal) {
    j["q_comp"] = static_cast<int>(b.mQuantize.mComp.normal);
  }
  if constexpr (kind == librii::gx::VertexBufferKind::textureCoordinate) {
    j["q_comp"] = static_cast<int>(b.mQuantize.mComp.texcoord);
  }
  if constexpr (kind == librii::gx::VertexBufferKind::color) {
    j["q_comp"] = static_cast<int>(b.mQuantize.mComp.color);
  }
  if constexpr (kind == librii::gx::VertexBufferKind::position) {
    j["q_type"] = static_cast<int>(b.mQuantize.mType.generic);
  }
  if constexpr (kind == librii::gx::VertexBufferKind::normal) {
    j["q_type"] = static_cast<int>(b.mQuantize.mType.generic);
  }
  if constexpr (kind == librii::gx::VertexBufferKind::textureCoordinate) {
    j["q_type"] = static_cast<int>(b.mQuantize.mType.generic);
  }
  if constexpr (kind == librii::gx::VertexBufferKind::color) {
    j["q_type"] = static_cast<int>(b.mQuantize.mType.color);
  }
  j["q_divisor"] = b.mQuantize.divisor;
  j["q_stride"] = b.mQuantize.stride;
  j["dataBufferId"] = c.save_buffer_with_copy<T>(b.mEntries);
}
void WriteJson(JsonWriteCtx& c, nlohmann::json& j,
               const librii::g3d::PolygonData& p) {
  j["name"] = p.mName;
  j["current_matrix"] = p.mCurrentMatrix;
  j["visible"] = p.visible;
  j["pos_buffer"] = p.mPositionBuffer;
  j["nrm_buffer"] = p.mNormalBuffer;
  j["clr_buffer"] = p.mColorBuffer;
  j["uv_buffer"] = p.mTexCoordBuffer;

  j["vcd"] = p.mVertexDescriptor.mBitfield;
  j["mprims"] = nlohmann::json::array();
  for (auto& mp : p.mMatrixPrimitives) {
    nlohmann::json tmp;

    tmp["matrices"] = mp.mDrawMatrixIndices;

    std::vector<u8> vd_buf;

    for (auto& prim : mp.mPrimitives) {
      vd_buf.push_back(static_cast<u8>(prim.mType));
      u32 num_verts = prim.mVertices.size();
      vd_buf.push_back((num_verts >> 16) & 0xff);
      vd_buf.push_back((num_verts >> 8) & 0xff);
      vd_buf.push_back((num_verts >> 0) & 0xff);
      // 26x u16s
      static_assert(sizeof(librii::gx::IndexedVertex) == 26 * 2);
      static_assert(alignof(librii::gx::IndexedVertex) == 2);
      u32 cursor = vd_buf.size();
      u32 buf_size = prim.mVertices.size() * sizeof(librii::gx::IndexedVertex);
      vd_buf.resize(vd_buf.size() + buf_size);
      memcpy(vd_buf.data() + cursor, prim.mVertices.data(), buf_size);
    }

    tmp["vertexDataBufferId"] = c.save_buffer_with_move(std::move(vd_buf));
    tmp["num_prims"] = mp.mPrimitives.size();

    j["mprims"].push_back(tmp);
  }
}
void WriteJson(JsonWriteCtx& ctx, nlohmann::json& j,
               const librii::g3d::Model& model) {

  j["name"] = model.name;
  // WriteJson(ctx, model.info, j["info"]); // Assuming a WriteJson function for
  // ModelInfo exists

  j["bones"] = nlohmann::json::array();
  for (const auto& bone : model.bones) {
    nlohmann::json boneJson;
    // WriteJson(ctx, bone, boneJson); // Assuming a WriteJson function for
    // BoneData exists
    j["bones"].push_back(boneJson);
  }

  j["positions"] = nlohmann::json::array();
  for (const auto& pos : model.positions) {
    nlohmann::json positionJson;
    WriteJson(ctx, positionJson, pos);
    j["positions"].push_back(positionJson);
  }

  j["normals"] = nlohmann::json::array();
  for (const auto& normal : model.normals) {
    nlohmann::json normalJson;
    WriteJson(ctx, normalJson, normal);
    j["normals"].push_back(normalJson);
  }

  j["colors"] = nlohmann::json::array();
  for (const auto& color : model.colors) {
    nlohmann::json colorJson;
    WriteJson(ctx, colorJson, color);
    j["colors"].push_back(colorJson);
  }

  j["texcoords"] = nlohmann::json::array();
  for (const auto& texcoord : model.texcoords) {
    nlohmann::json texcoordJson;
    WriteJson(ctx, texcoordJson, texcoord);
    j["texcoords"].push_back(texcoordJson);
  }

  j["materials"] = nlohmann::json::array();
  for (const auto& material : model.materials) {
    nlohmann::json materialJson;
    // WriteJson(ctx, material, materialJson);
    j["materials"].push_back(materialJson);
  }

  j["meshes"] = nlohmann::json::array();
  for (const auto& mesh : model.meshes) {
    nlohmann::json meshJson;
    WriteJson(ctx, meshJson, mesh);
    j["meshes"].push_back(meshJson);
  }

  j["matrices"] = nlohmann::json::array();
  for (const auto& matrix : model.matrices) {
    nlohmann::json matrixJson;
    // WriteJson(ctx, matrix, matrixJson);
    j["matrices"].push_back(matrixJson);
  }
}

void WriteJson(JsonWriteCtx& ctx, nlohmann::json& j,
               const librii::g3d::Archive& archive) {
  j["models"] = nlohmann::json::array();
  for (const auto& model : archive.models) {
    nlohmann::json modelJson;
    WriteJson(ctx, modelJson, model);
    j["models"].push_back(modelJson);
  }

  j["textures"] = nlohmann::json::array();
  for (const auto& texture : archive.textures) {
    nlohmann::json textureJson;
    WriteJson(ctx, textureJson, texture);
    j["textures"].push_back(textureJson);
  }

  j["chrs"] = nlohmann::json::array();
  for (const auto& chr : archive.chrs) {
    nlohmann::json chrJson;
    // WriteJson(ctx, chr, chrJson); // Assuming a WriteJson function for
    // BinaryChr exists
    j["chrs"].push_back(chrJson);
  }

  j["clrs"] = nlohmann::json::array();
  for (const auto& clr : archive.clrs) {
    nlohmann::json clrJson;
    // WriteJson(ctx, clr, clrJson); // Assuming a WriteJson function for
    // BinaryClr exists
    j["clrs"].push_back(clrJson);
  }

  j["pats"] = nlohmann::json::array();
  for (const auto& pat : archive.pats) {
    nlohmann::json patJson;
    // WriteJson(ctx, pat, patJson); // Assuming a WriteJson function for
    // BinaryTexPat exists
    j["pats"].push_back(patJson);
  }

  j["srts"] = nlohmann::json::array();
  for (const auto& srt : archive.srts) {
    nlohmann::json srtJson;
    // WriteJson(ctx, srt, srtJson); // Assuming a WriteJson function for
    // SrtAnim exists
    j["srts"].push_back(srtJson);
  }

  j["viss"] = nlohmann::json::array();
  for (const auto& vis : archive.viss) {
    nlohmann::json visJson;
    // WriteJson(ctx, vis, visJson); // Assuming a WriteJson function for
    // BinaryVis exists
    j["viss"].push_back(visJson);
  }
}

std::vector<u8> CollateBuffers(const JsonWriteCtx& c) {
  std::vector<u8> result;
  /*
  clang-format off

  big endian

  Format:
  u32 magic; // RBUF
  u32 version; // 100
  u32 num_buffers;
  u32 file_size;

  // for 0..num_buffers:
	u32buffer_offset
	u32 buffer_size

  // Buffers begin...

  clang-format on
  */
  auto write_u32_at = [&](u32 u, u32 pos) {
    result[pos] = u >> 24;
    result[pos + 1] = u >> 16;
    result[pos + 2] = u >> 8;
    result[pos + 3] = u >> 0;
  };
  u32 size = 16 + 8 * c.buffers.size();
  u32 buffer_cursor = roundUp(size, 64);
  for (auto& buf : c.buffers) {
    // All entries 64-byte aligned
    size = roundUp(size, 64);
    size += buf.size();
  }
  size = roundUp(size, 64);
  result.resize(size);

  result[0] = 'R';
  result[1] = 'B';
  result[2] = 'U';
  result[3] = 'F';

  write_u32_at(100, 4);
  write_u32_at(c.buffers.size(), 8);
  write_u32_at(size, 12);
  u32 cursor = 16;
  for (auto& buf : c.buffers) {
    write_u32_at(buffer_cursor, cursor);
    write_u32_at(buf.size(), cursor + 4);
    cursor += 8;
    memcpy(result.data() + buffer_cursor, buf.data(), buf.size());
    buffer_cursor += buf.size();
    buffer_cursor = roundUp(buffer_cursor, 64);
  }
  assert(buffer_cursor == size);
  return result;
}

void TestJson(const librii::g3d::Archive& archive) {
  JsonWriteCtx ctx;
  nlohmann::json j;
  WriteJson(ctx, j, archive);
  std::cout << j << std::endl;
  std::print(std::cout, "Number of buffers: {}\n", ctx.buffers.size());
  auto collated = CollateBuffers(ctx);
  std::print(std::cout, "filesize of raw data: {}\n", collated.size());
}

struct DumpResult {
  std::string jsonData;
  std::vector<u8> collatedBuffer;
};

DumpResult DumpJson(const librii::g3d::Archive& archive) {
  JsonWriteCtx ctx;
  nlohmann::json j;
  WriteJson(ctx, j, archive);
  auto collated = CollateBuffers(ctx);
  return DumpResult{j.dump(), std::move(collated)};
}
