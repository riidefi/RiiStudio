#include "RsMeshOpt.hpp"
#include "HaroohieTriStripifier.hpp"
#include "TriFanMeshOptimizer.hpp"
#include "TriStripper/tri_stripper.h"
#include "draco/mesh/mesh_stripifier.h"
#include "meshoptimizer/meshoptimizer.h"
#include "tristrip/tristrip.hpp"

#include <rsl/Expect.hpp>
#include <rsl/Result.hpp>
#include <rsl/Try.hpp>

namespace rsmeshopt {

Result<std::vector<std::vector<u32>>>
StripifyTrianglesNvTriStripPort(std::span<const u32> index_data) {
  EXPECT(index_data.size() % 3 == 0);
  std::list<std::list<int>> mesh;
  for (size_t i = 0; i < index_data.size(); i += 3) {
    std::list<int> tri;
    tri.push_back(index_data[i]);
    tri.push_back(index_data[i + 1]);
    tri.push_back(index_data[i + 2]);
    mesh.push_back(std::move(tri));
  }
  std::list<std::deque<int>> strips = stripify(mesh);

  std::vector<std::vector<u32>> result;
  for (auto& s : strips) {
    auto& tmp = result.emplace_back();
    for (auto v : s) {
      EXPECT(v >= 0);
      tmp.push_back(static_cast<u32>(v));
    }
  }
  return result;
}

Result<std::vector<u32>>
StripifyTrianglesNvTriStripPort2(std::span<const u32> index_data, u32 restart) {
  auto v = TRY(StripifyTrianglesNvTriStripPort(index_data));
  std::vector<u32> result;
  for (auto& x : v) {
    for (size_t i = 0; i < x.size(); ++i) {
      result.push_back(x[i]);
    }
    result.push_back(restart);
  }
  if (result.size()) {
    result.resize(result.size() - 1);
  }
  return result;
}

Result<triangle_stripper::primitive_vector>
StripifyTrianglesTriStripper(std::span<const u32> index_data) {
  EXPECT(index_data.size() % 3 == 0);
  std::vector<size_t> buffer(index_data.begin(), index_data.end());

  triangle_stripper::tri_stripper stripper(buffer);
  triangle_stripper::primitive_vector out;
  stripper.Strip(&out);
  return out;
}

Result<std::vector<u32>>
StripifyTrianglesTriStripper2(std::span<const u32> index_data, u32 restart) {
  auto v = TRY(StripifyTrianglesTriStripper(index_data));
  std::vector<u32> result;
  for (auto& x : v) {
    if (x.Type == triangle_stripper::TRIANGLES) {
      for (size_t i = 0; i < x.Indices.size(); ++i) {
        result.push_back(x.Indices[i]);
        if (i != 0 && ((i + 1) % 3) == 0) {
          result.push_back(restart);
        }
      }
    } else if (x.Type == triangle_stripper::TRIANGLE_STRIP) {
      for (size_t i = 0; i < x.Indices.size(); ++i) {
        result.push_back(x.Indices[i]);
      }
    }
    result.push_back(restart);
  }
  if (result.size()) {
    result.resize(result.size() - 1);
  }
  return result;
}

size_t meshopt_stripify(unsigned int* destination, const unsigned int* indices,
                        size_t index_count, size_t vertex_count,
                        unsigned int restart_index) {
  return ::meshopt_stripify(destination, indices, index_count, vertex_count,
                            restart_index);
}
size_t meshopt_stripifyBound(size_t index_count) {
  return ::meshopt_stripifyBound(index_count);
}
size_t meshopt_unstripify(unsigned int* destination,
                          const unsigned int* indices, size_t index_count,
                          unsigned int restart_index) {
  return ::meshopt_unstripify(destination, indices, index_count, restart_index);
}
size_t meshopt_unstripifyBound(size_t index_count) {
  return ::meshopt_unstripifyBound(index_count);
}

std::vector<u32> StripifyMeshOpt(std::span<const u32> index_data,
                                 u32 vertex_count, u32 restart) {
  std::vector<u32> dst(meshopt_stripifyBound(index_data.size()));
  static_assert(sizeof(u32) == sizeof(unsigned int));
  size_t num = meshopt_stripify((unsigned int*)dst.data(),
                                (const unsigned int*)index_data.data(),
                                index_data.size(), vertex_count, restart);
  assert(num <= dst.size());
  dst.resize(num);
  return dst;
}

Result<std::vector<u32>> StripifyDraco(std::span<const u32> index_data,
                                       std::span<const vec3> vertex_data,
                                       u32 restart, bool degen) {
  auto mesh = std::make_shared<draco::Mesh>();
  for (size_t i = 0; i < index_data.size(); i += 3) {
    std::array<draco::PointIndex, 3> face;
    face[0] = index_data[i];
    face[1] = index_data[i + 1];
    face[2] = index_data[i + 2];
    mesh->AddFace(face);
  }
  auto pos = std::make_unique<draco::PointAttribute>();
  pos->Init(draco::GeometryAttribute::POSITION, 3, draco::DT_FLOAT32, false,
            vertex_data.size());
  auto other = std::make_unique<draco::PointAttribute>();
  other->Init(draco::GeometryAttribute::GENERIC, 1, draco::DT_UINT32, false,
              vertex_data.size());
  for (size_t i = 0; i < vertex_data.size(); ++i) {
    pos->SetAttributeValue(draco::AttributeValueIndex(i), &vertex_data[i]);
    u32 tmp = i;
    other->SetAttributeValue(draco::AttributeValueIndex(i), &tmp);
  }
  mesh->SetAttribute(0, std::move(pos));
  mesh->SetAttribute(1, std::move(other));

  std::vector<u32> stripified;
  draco::MeshStripifier stripifier;
  bool ok = false;
  if (degen) {
    ok = stripifier.GenerateTriangleStripsWithDegenerateTriangles(
        *mesh, std::back_inserter(stripified));
  } else {
    ok = stripifier.GenerateTriangleStripsWithPrimitiveRestart(
        *mesh, restart, std::back_inserter(stripified));
  }
  EXPECT(ok);
  return stripified;
}

std::expected<std::vector<u32>, std::string>
MakeFans(std::span<const u32> index_data, u32 restart, u32 min_len,
         u32 max_runs) {
  rsmeshopt::TriFanMeshOptimizer fan_pass;
  rsmeshopt::TriFanOptions options{min_len, max_runs};
  std::vector<u32> stripified;
  bool ok = fan_pass.GenerateTriangleFansWithPrimitiveRestart(
      index_data, ~0u, std::back_inserter(stripified), options);
  EXPECT(ok);
  return stripified;
}

Result<std::vector<u32>> StripifyHaroohie(std::span<const u32> index_data,
                                          u32 restart) {
  HaroohiePals::TriangleStripifier stripifier;
  std::vector<u32> result;
  auto ok = stripifier.GenerateTriangleStripsWithPrimitiveRestart(
      index_data, ~0u, std::back_inserter(result));
  EXPECT(ok);
  return result;
}

std::expected<std::vector<u32>, std::string>
DoStripifyAlgo(StripifyAlgo algo, std::span<const u32> index_data,
               std::span<const vec3> vertex_data, u32 restart) {
  switch (algo) {
  case StripifyAlgo::NvTriStripPort:
    return StripifyTrianglesNvTriStripPort2(index_data, restart);
  case StripifyAlgo::TriStripper:
    return StripifyTrianglesTriStripper2(index_data, restart);
  case StripifyAlgo::MeshOpt:
    return StripifyMeshOpt(index_data, vertex_data.size(), restart);
  case StripifyAlgo::Draco:
    return StripifyDraco(index_data, vertex_data, restart, false);
  case StripifyAlgo::DracoDegen:
    return StripifyDraco(index_data, vertex_data, restart, true);
  case StripifyAlgo::Haroohie:
    return StripifyHaroohie(index_data, restart);
  }

  EXPECT(false && "Invalid algorithm!");
}

} // namespace rsmeshopt
