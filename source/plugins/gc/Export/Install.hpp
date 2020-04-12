#pragma once

//	namespace libcube {
//	    void Install();
//	}

#include <core/common.h>
#include <core/kpi/Node.hpp>

#include "Bone.hpp"
#include "IndexedPolygon.hpp"
#include "Material.hpp"
#include "Texture.hpp"

namespace libcube {

namespace UI {
void installDisplaySurface();
}

static inline void Install() {
  assert(kpi::ApplicationPlugins::getInstance());
  kpi::ApplicationPlugins &installer = *kpi::ApplicationPlugins::getInstance();

  installer.registerParent<libcube::IBoneDelegate, riistudio::lib3d::Bone>()
      .registerParent<libcube::IGCMaterial, riistudio::lib3d::Material>()
      .registerParent<libcube::Texture, riistudio::lib3d::Texture>()
      .registerParent<libcube::IndexedPolygon, riistudio::lib3d::Polygon>();

  UI::installDisplaySurface();
}

} // namespace libcube
