#include <vendor/glfw/glfw3.h>

#include "GXMaterial.hpp"

namespace libcube {

using namespace gx;

const std::array<VertexAttributeGenDef, 15> vtxAttributeGenDefs {
    VertexAttributeGenDef { VertexAttribute::Position,						"Position",      GL_FLOAT,          3 },
    VertexAttributeGenDef { VertexAttribute::PositionNormalMatrixIndex,		"PnMtxIdx",      GL_FLOAT,			1 },
    VertexAttributeGenDef { VertexAttribute::Texture0MatrixIndex,			"TexMtx0123Idx", GL_FLOAT,			4 },
    VertexAttributeGenDef { VertexAttribute::Texture4MatrixIndex,			"TexMtx4567Idx", GL_FLOAT,			4 },
    VertexAttributeGenDef { VertexAttribute::Normal,						"Normal",        GL_FLOAT,          3 },
    VertexAttributeGenDef { VertexAttribute::Color0,						"Color0",        GL_FLOAT,          4 },
    VertexAttributeGenDef { VertexAttribute::Color1,						"Color1",        GL_FLOAT,          4 },
    VertexAttributeGenDef { VertexAttribute::TexCoord0,						"Tex0",          GL_FLOAT,          2 },
    VertexAttributeGenDef { VertexAttribute::TexCoord1,						"Tex1",          GL_FLOAT,          2 },
    VertexAttributeGenDef { VertexAttribute::TexCoord2,						"Tex2",          GL_FLOAT,          2 },
    VertexAttributeGenDef { VertexAttribute::TexCoord3,						"Tex3",          GL_FLOAT,          2 },
    VertexAttributeGenDef { VertexAttribute::TexCoord4,						"Tex4",          GL_FLOAT,          2 },
    VertexAttributeGenDef { VertexAttribute::TexCoord5,						"Tex5",          GL_FLOAT,          2 },
    VertexAttributeGenDef { VertexAttribute::TexCoord6,						"Tex6",          GL_FLOAT,          2 },
    VertexAttributeGenDef { VertexAttribute::TexCoord7,						"Tex7",          GL_FLOAT,          2 }
};

std::pair<const VertexAttributeGenDef&, std::size_t> getVertexAttribGenDef(VertexAttribute vtxAttrib)
{
    const auto it = std::find_if(vtxAttributeGenDefs.begin(), vtxAttributeGenDefs.end(), [vtxAttrib](const VertexAttributeGenDef& def) {
        return def.attrib == vtxAttrib;
    });

    assert(it != vtxAttributeGenDefs.end());

	return { *it, it - vtxAttributeGenDefs.begin() };
}
// TODO: Unify?
int getVertexAttribLocation(VertexAttribute vtxAttrib)
{
    const auto it = std::find_if(vtxAttributeGenDefs.begin(), vtxAttributeGenDefs.end(), [vtxAttrib](const VertexAttributeGenDef& def) {
        return def.attrib == vtxAttrib;
    });

    assert(it != vtxAttributeGenDefs.end());

    return static_cast<int>(std::distance(vtxAttributeGenDefs.begin(), it));
}
std::string generateBindingsDefinition(bool postTexMtxBlock, bool lightsBlock)
{
#ifdef _WIN32
    return std::string(R"(
// Expected to be constant across the entire scene.
layout(std140, binding=0) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

#define u_SceneTextureLODBias u_Misc0[0]
struct Light {
    vec4 Color;
    vec4 Position;
    vec4 Direction;
    vec4 DistAtten;
    vec4 CosAtten;
};
// Expected to change with each material.
layout(std140, row_major, binding=1) uniform ub_MaterialParams {
    vec4 u_ColorMatReg[2];
    vec4 u_ColorAmbReg[2];
    vec4 u_KonstColor[4];
    vec4 u_Color[4];
    mat4x3 u_TexMtx[10]; //4x3
    // SizeX, SizeY, 0, Bias
    vec4 u_TextureParams[8];
    mat4x2 u_IndTexMtx[3]; // 4x2
    // Optional parameters.)") + "\n" +
(postTexMtxBlock ? "Mat4x3 u_PostTexMtx[20];\n" : "") + // 4x3
(lightsBlock ? "Light u_LightParams[8];\n" : "") +
std::string("};\n") +
"// Expected to change with each shape packet.\n"
"layout(std140, row_major, binding=2) uniform ub_PacketParams {\n"
"    mat4x3 u_PosMtx[10];\n" // 4x3
"};\n"
"uniform sampler2D u_Texture[8];\n";
#else
	    return std::string(R"(
// Expected to be constant across the entire scene.
layout(std140) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

#define u_SceneTextureLODBias u_Misc0[0]
struct Light {
    vec4 Color;
    vec4 Position;
    vec4 Direction;
    vec4 DistAtten;
    vec4 CosAtten;
};
// Expected to change with each material.
layout(std140, row_major) uniform ub_MaterialParams {
    vec4 u_ColorMatReg[2];
    vec4 u_ColorAmbReg[2];
    vec4 u_KonstColor[4];
    vec4 u_Color[4];
    mat4x3 u_TexMtx[10]; //4x3
    // SizeX, SizeY, 0, Bias
    vec4 u_TextureParams[8];
    mat4x2 u_IndTexMtx[3]; // 4x2
    // Optional parameters.)") + "\n" +
(postTexMtxBlock ? "Mat4x3 u_PostTexMtx[20];\n" : "") + // 4x3
(lightsBlock ? "Light u_LightParams[8];\n" : "") +
std::string("};\n") +
"// Expected to change with each shape packet.\n"
"layout(std140, row_major) uniform ub_PacketParams {\n"
"    mat4x3 u_PosMtx[10];\n" // 4x3
"};\n"
"uniform sampler2D u_Texture[8];\n";
#endif
}

} // namespace libcube
