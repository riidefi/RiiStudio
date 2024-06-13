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

#include <rsl/WriteFile.hpp>

template <typename T> using Expected = std::expected<T, std::string>;

struct JsonWriteCtx;

namespace librii::g3d {
std::string ArcToJSON(const g3d::Archive& model, JsonWriteCtx& c);
Result<g3d::Archive> JSONToArc(std::string_view json,
                               std::span<const std::vector<u8>> buffers);
} // namespace librii::g3d

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

void WriteJson(JsonWriteCtx& ctx, nlohmann::json& j,
               const librii::g3d::Archive& archive) {
  auto s = librii::g3d::ArcToJSON(archive, ctx);
  j = nlohmann::json::parse(s);
}

constexpr u32 RBUF_ENTRY_ALIGNMENT = 64;

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
u32 read_u32_at(std::span<const u8> data, u32 pos) {
  return (data[pos] << 24) | (data[pos + 1] << 16) | (data[pos + 2] << 8) |
         (data[pos + 3]);
}

std::vector<std::vector<u8>> ParseBuffers(std::span<const u8> data) {
  std::vector<std::vector<u8>> buffers;

  // Check the magic number
  assert(data[0] == 'R' && data[1] == 'B' && data[2] == 'U' && data[3] == 'F');

  u32 version = read_u32_at(data, 4);
  assert(version == 100);

  u32 num_buffers = read_u32_at(data, 8);
  u32 file_size = read_u32_at(data, 12);

  u32 cursor = 16;

  for (u32 i = 0; i < num_buffers; ++i) {
    u32 buffer_offset = read_u32_at(data, cursor);
    u32 buffer_size = read_u32_at(data, cursor + 4);
    cursor += 8;

    std::vector<u8> buffer(buffer_size);
    std::memcpy(buffer.data(), data.data() + buffer_offset, buffer_size);
    buffers.push_back(buffer);
  }

  return buffers;
}

void TestJson(const librii::g3d::Archive& archive) {
  JsonWriteCtx ctx;
  nlohmann::json j;
  WriteJson(ctx, j, archive);
  std::cout << j << std::endl;
  std::print(std::cout, "Number of buffers: {}\n", ctx.buffers.size());
  auto collated = CollateBuffers(ctx);
  std::print(std::cout, "filesize of raw data: {}\n", collated.size());
  (void)rsl::WriteFile(collated, "Yea.bin");
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
  return DumpResult{j.dump(4), std::move(collated)};
}
Result<librii::g3d::Archive> ReadJsonArc(std::string_view json,
                                         std::span<const u8> buffer) {
  auto buffers = ParseBuffers(buffer);
  return librii::g3d::JSONToArc(json, buffers);
}
Result<std::vector<u8>> WriteArchive(std::string_view json,
                                     std::span<const u8> blob) {
  auto arc = TRY(ReadJsonArc(json, blob));
  auto bin = TRY(arc.write());

  return bin;
}
