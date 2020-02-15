#include "export.hpp"

#include <LibCore/common.h>
#include <LibCore/api/Node.hpp>

#include <Lib3D/io/fbx.hpp>
#include <Lib3D/interface/i3dmodel.hpp>

namespace lib3d {

void install()
{
	assert(px::PackageInstaller::spInstance);
	px::PackageInstaller& installer = *px::PackageInstaller::spInstance;
	installer.registerSerializer(std::make_unique<FbxIo>());

	installer.registerObject(Scene::TypeInfo);
	installer.registerParent<Scene, px::CollectionHost>();
}

} // namespace lib3d
