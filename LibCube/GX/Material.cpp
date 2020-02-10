#include <ThirdParty/glfw/glfw3.h>


#include "Material.hpp"

#include <LibCube/GX/Shader/GXProgram.hpp>

namespace libcube {

std::pair<std::string, std::string> IMaterialDelegate::generateShaders() const
{
	GXProgram program(GXMaterial{ 0, getName(), *const_cast<IMaterialDelegate*>(this) });
	return program.generateShaders();
}

}
