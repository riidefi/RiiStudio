#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

//
// Holds the following dictionaries:
// { (MatrixId, TargetID) : bool }
//   e.g. (IndMtx0, SCALE_U) -> true
// { MaterialName : bool }
//   e.g. "sea" -> false
//
class Filter {
public:
  struct Attr {
    bool ind = false;
    size_t matrix = 0;
    enum {
      MAX_ATTRS = 8 + 3,
    };
    size_t index() const { return matrix + static_cast<size_t>(ind) * 8; }
  };

  Result<void> initFromScene(const g3d::Model& model,
                             const librii::g3d::SrtAnim& srt) {
    m_materials.clear();
	// For now just show all always.
    for (auto& a : m_attributes) {
      a = true;
	}
    for (auto& mat : model.getMaterials()) {
      m_materials[mat.name] = false;
    }
    for (auto& mtx : srt.matrices) {
      auto mat = mtx.target.materialName;
      EXPECT(m_materials.contains(mat));
      m_materials[mat] = true;
      Attr a{
          .ind = mtx.target.indirect,
          .matrix = static_cast<size_t>(mtx.target.matrixIndex),
      };
      using T = librii::g3d::SRT0Matrix::TargetId;
      for (auto t : {T::ScaleU, T::ScaleV, T::Rotate, T::TransU, T::TransV}) {
        *attr(a.index()) = true;
      }
    }
    return {};
  }
  bool* material(std::string m) {
    auto it = m_materials.find(m);
    if (it == m_materials.end()) {
      return nullptr;
    }
    return &it->second;
  }
  bool* attr(size_t idx) { return &m_attributes[idx]; }
  std::map<std::string, bool>& materials() { return m_materials; }

private:
  std::map<std::string, bool> m_materials{};
  // was std::map<librii::g3d::SRT0Matrix::TargetId, bool>
  std::array<bool, Attr::MAX_ATTRS> m_attributes{};
};

struct G3dSrtOptionsSurface {
  static inline const char* name() { return "Options"_j; }
  static inline const char* icon = (const char*)ICON_FA_COG;

  bool m_filterReady = false; // TODO: Use a UUID for SRT0
  Filter m_filter;
};

void drawProperty(kpi::PropertyDelegate<SRT0>& dl,
                  G3dSrtOptionsSurface& surface);

} // namespace riistudio::g3d
