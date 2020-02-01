#include "Install.hpp"

#include <LibCore/api/Node.hpp>
#include <LibCore/common.h>

#include "Bone.hpp"
#include "Material.hpp"

namespace libcube {

void Install()
{
    assert(px::PackageInstaller::spInstance);
    px::PackageInstaller& installer = *px::PackageInstaller::spInstance;

    installer.registerParent<IBoneDelegate, lib3d::Bone>();
    installer.registerParent<IMaterialDelegate, lib3d::Material>();
}

}