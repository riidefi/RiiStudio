#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <rsl/SmallVector.hpp>

namespace librii::g3d {

struct TextureSamplerMapping {
  TextureSamplerMapping() = default;
  TextureSamplerMapping(std::string n, u32 pos) : name(n) {
    entries.emplace_back(pos);
  }

  const std::string& getName() const { return name; }
  std::string name;
  rsl::small_vector<u32, 1> entries; // stream position
};
struct TextureSamplerMappingManager {
  void add_entry(const std::string& name, const G3dMaterialData* mat,
                 int mat_sampl_index) {
    if (auto found = std::find_if(entries.begin(), entries.end(),
                                  [name](auto& e) { return e.name == name; });
        found != entries.end()) {
      matMap[{mat, mat_sampl_index}] = {found - entries.begin(),
                                        found->entries.size()};
      found->entries.emplace_back(0);
      return;
    }
    matMap[{mat, mat_sampl_index}] = {entries.size(), 0};
    entries.emplace_back(name, 0);
  }
  std::pair<u32, u32> from_mat(const G3dMaterialData* mat, int sampl_idx) {
    std::pair key{mat, sampl_idx};
    if (!matMap.contains(key)) {
      // TODO: Invalid tex ref.
      return {0, 0};
    }
    assert(matMap.contains(key));
    const auto [entry_n, sub_n] = matMap.at(key);
    const auto entry_ofs = entries[entry_n].entries[sub_n];
    return {entry_ofs, entry_ofs - sub_n * 8 - 4};
  }
  std::vector<TextureSamplerMapping> entries;
  std::map<std::pair<const G3dMaterialData*, int>, std::pair<int, int>> matMap;

  auto begin() { return entries.begin(); }
  auto end() { return entries.end(); }
  std::size_t size() const { return entries.size(); }
  TextureSamplerMapping& operator[](std::size_t i) { return entries[i]; }
  const TextureSamplerMapping& operator[](std::size_t i) const {
    return entries[i];
  }
};

struct ShaderAllocator {
  void alloc(const librii::g3d::G3dShader& shader) {
    auto found = std::find(shaders.begin(), shaders.end(), shader);
    if (found == shaders.end()) {
      matToShaderMap.emplace_back(shaders.size());
      shaders.emplace_back(shader);
    } else {
      matToShaderMap.emplace_back(found - shaders.begin());
    }
  }

  int find(const librii::g3d::G3dShader& shader) const {
    if (const auto found = std::find(shaders.begin(), shaders.end(), shader);
        found != shaders.end()) {
      return std::distance(shaders.begin(), found);
    }

    return -1;
  }

  auto size() const { return shaders.size(); }

  std::string getShaderIDName(const librii::g3d::G3dShader& shader) const {
    const int found = find(shader);
    assert(found >= 0);
    return "Shader" + std::to_string(found);
  }

  std::vector<librii::g3d::G3dShader> shaders;
  std::vector<u32> matToShaderMap;
};

bool readMaterial(G3dMaterialData& mat, oishii::BinaryReader& reader,
                  bool ignore_tev = false);

void WriteMaterial(const size_t& mat_start, oishii::Writer& writer,
                   NameTable& names, const G3dMaterialData& mat, u32& mat_idx,
                   RelocWriter& linker, const ShaderAllocator& shader_allocator,
                   TextureSamplerMappingManager& tex_sampler_mappings);

} // namespace librii::g3d
