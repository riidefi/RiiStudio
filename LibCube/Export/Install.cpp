#include "Install.hpp"

#include <LibCore/api/Node.hpp>
#include <LibCore/common.h>

#include "Bone.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "IndexedPolygon.hpp"

namespace libcube {

void Install()
{
    assert(px::PackageInstaller::spInstance);
    px::PackageInstaller& installer = *px::PackageInstaller::spInstance;

    installer.registerParent<IBoneDelegate, lib3d::Bone>();
    installer.registerParent<IMaterialDelegate, lib3d::Material>();
	installer.registerParent<Texture, lib3d::Texture>();
	installer.registerParent<IndexedPolygon, lib3d::Polygon>();
}

}
