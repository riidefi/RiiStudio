#include <ThirdParty/glfw/glfw3.h>


#include "Material.hpp"

#include <LibCube/GX/Shader/GXProgram.hpp>

namespace libcube {

std::pair<std::string, std::string> IMaterialDelegate::generateShaders() const
{
	GXProgram program(GXMaterial{ 0, getName(), *const_cast<IMaterialDelegate*>(this) });
	return program.generateShaders();
}


/*
Layout in memory:
(Binding 0) Scene
(Binding 1) Mat
(Binding 2) Shape

<---
Scene
Mat
<---
Mat
Mat
Shape
<---
Shape
Shape

*/

struct UniformSceneParams
{
	glm::mat4 projection;
	glm::vec4 Misc0;
};
// ROW_MAJOR
struct PacketParams
{
	glm::mat3x4	posMtx[10];
};
void IMaterialDelegate::generateUniforms(DelegatedUBOBuilder& builder, const glm::mat4& M, const glm::mat4& V, const glm::mat4& P) const
{
	UniformSceneParams scene;
	scene.projection = M * V * P;
	scene.Misc0 = {};

	UniformMaterialParams tmp{};

	PacketParams pack{};
	for (auto& p : pack.posMtx)
		p = glm::transpose(glm::mat4{ 1.0f });

	builder.tpush(0, scene);
	builder.tpush(1, tmp);
	builder.tpush(2, pack);
}

}
