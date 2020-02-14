#include <ThirdParty/glfw/glfw3.h>


#include "Material.hpp"

#include <LibCube/GX/Shader/GXProgram.hpp>

namespace libcube {

std::pair<std::string, std::string> IGCMaterial::generateShaders() const
{
	GXProgram program(GXMaterial{ 0, getName(), *const_cast<IGCMaterial*>(this) });
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
template<typename T>
inline glm::vec4 colorConvert(T clr)
{
	const auto f32c = (gx::ColorF32)clr;
	return { f32c.r, f32c.g, f32c.b, f32c.a };
}
void IGCMaterial::generateUniforms(DelegatedUBOBuilder& builder, const glm::mat4& M, const glm::mat4& V, const glm::mat4& P) const
{
	UniformSceneParams scene;
	scene.projection = M * V * P;
	scene.Misc0 = {};

	UniformMaterialParams tmp{};

	const auto& data = getMaterialData();

	for (int i = 0; i < 2; ++i)
	{
		tmp.ColorMatRegs[i] = colorConvert(data.chanData[i].matColor);
		tmp.ColorAmbRegs[i] = colorConvert(data.chanData[i].ambColor);
	}
	for (int i = 0; i < 4; ++i)
	{
		tmp.KonstColor[i] = colorConvert(data.tevKonstColors[i]);
		tmp.Color[i] = colorConvert(data.tevColors[i]);
	}
	for (int i = 0; i < data.texMatrices.size(); ++i)
	{
		tmp.TexMtx[i] = data.texMatrices[i]->compute();
	}
	for (int i = 0; i < data.samplers.size(); ++i)
	{
		const auto& texData = data.samplers[i]->mTexture;
		
		// tmp.TexParams[i] = glm::vec4{ texData.getWidth(), texData.getHeight(), 0, 0 };
	}
	for (int i = 0; i < data.mIndMatrices.size(); ++i)
	{
		tmp.IndTexMtx[i] = data.mIndMatrices[i].compute();
	}
	PacketParams pack{};
	for (auto& p : pack.posMtx)
		p = glm::transpose(glm::mat4{ 1.0f });

	builder.tpush(0, scene);
	builder.tpush(1, tmp);
	builder.tpush(2, pack);
}

}
