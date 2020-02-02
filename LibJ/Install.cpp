#include "Install.hpp"

#include <LibCore/api/Node.hpp>
#include <LibCore/common.h>

#include <LibJ/J3D/Collection.hpp>

#include <LibJ/J3D/Binary/BMD/BMD.hpp>

namespace libcube::jsystem {

void Install()
{
    
    assert(px::PackageInstaller::spInstance);
    px::PackageInstaller& installer = *px::PackageInstaller::spInstance;

    installer.registerParent<J3DCollection, px::CollectionHost>();

	installer.registerObject(J3DCollection::TypeInfo);
	installer.registerObject(J3DModel::TypeInfo);
	installer.registerObject(Material::TypeInfo);
	installer.registerObject(Joint::TypeInfo);
	installer.registerObject(Texture::TypeInfo);


    installer.registerParent<Joint, IBoneDelegate>();
    installer.registerParent<Material, IMaterialDelegate>();
	installer.registerParent<Texture, libcube::Texture>();

    // TODO: Move to proper system
    installer.registerParent<IBoneDelegate, lib3d::Bone>();
    installer.registerParent<Material, lib3d::Material>();

	installer.registerParent<J3DModel, px::CollectionHost>();

	installer.registerSerializer(std::make_unique<BmdIo>());
	installer.registerFactory(std::make_unique<J3DCollectionSpawner>());
}
    
}
