#include "Install.hpp"

#include <core/kpi/Node.hpp>
#include <core/common.h>

#include "Bone.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "IndexedPolygon.hpp"

namespace libcube {

void Install()
{
    assert(kpi::ApplicationPlugins::getInstance());
    kpi::ApplicationPlugins& installer = *kpi::ApplicationPlugins::getInstance();

    installer.registerParent<libcube::IBoneDelegate, riistudio::lib3d::Bone>();
    installer.registerParent<libcube::IGCMaterial, riistudio::lib3d::Material>();
	installer.registerParent<libcube::Texture, riistudio::lib3d::Texture>();
	installer.registerParent<libcube::IndexedPolygon, riistudio::lib3d::Polygon>();
}

}
