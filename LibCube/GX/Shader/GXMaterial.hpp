/*!
 * @file
 * @brief TODO
 */

#pragma once


#include <string>
#include <array>
#include <iterator>

#include <ThirdParty/glm/vec3.hpp>
#include <ThirdParty/glm/vec4.hpp>
#include <ThirdParty/glm/mat4x4.hpp>


#include <GL/GL.h>


#include <LibCube/Export/Material.hpp>
#include <LibCube/GX/VertexTypes.hpp>

typedef int TODO;

namespace libcube {

struct LightingChannelControl
{
	gx::ChannelControl colorChannel;
	gx::ChannelControl alphaChannel;

};

//! @brief Based on noclip's implementation.
//!
struct GXMaterial
{
    int index = -1; //!<
    std::string name; //!< For debugging.

	IMaterialDelegate& mat;

    bool usePnMtxIdx = false;
    bool useTexMtxIdx[16] {false};
    bool hasPostTexMtxBlock = false;
    bool hasLightsBlock = false;

    
    inline u32 calcParamsBlockSize() const noexcept
    {
        u32 size = 4*2 + 4*2 + 4*4 + 4*4 + 4*3*10 + 4*2*3 + 4*8;
        if (hasPostTexMtxBlock)
            size += 4*3*20;
        if (hasLightsBlock)
            size += 4*5*8;
        
        return size;
    }
};

static glm::vec4 scratchVec4;

//----------------------------------
// Lighting
struct Light
{
    glm::vec3 Position  = { 0, 0, 0 };
    glm::vec3 Direction = { 0, 0, -1 };
    glm::vec3 DistAtten = { 0, 0, 0 };
    glm::vec3 CosAtten  = { 0, 0, 0 };
    gx::ColorS10  Color     = { 0, 0, 0, 1};

    void setWorldPositionViewMatrix(const glm::mat4& viewMatrix, float x, float y, float z)
    {
        Position = glm::vec4{ x, y, z, 1.0f } * viewMatrix;
    }

    void setWorldDirectionNormalMatrix(const glm::mat4& normalMatrix, float x, float y, float z)
    {
        Direction = glm::normalize(glm::vec4(x, y, z, 0.0f) * normalMatrix);
    }

    
};

//----------------------------------
// Uniforms
enum class UniformStorage
{
    UINT,
    VEC2,
    VEC3,
    VEC4,
};

//----------------------------------
// Vertex attribute generation
struct VertexAttributeGenDef
{
    gx::VertexAttribute attrib;
    const char* name;
    
    u32 format;
    int size;
    
};

const VertexAttributeGenDef& getVertexAttribGenDef(gx::VertexAttribute vtxAttrib);
int getVertexAttribLocation(gx::VertexAttribute vtxAttrib);

std::string generateBindingsDefinition(bool postTexMtxBlock = false, bool lightsBlock = false);

extern const std::array<VertexAttributeGenDef, 15> vtxAttributeGenDefs;
struct UniformMaterialParams
{
	std::array<glm::vec4, 2> ColorMatRegs;
	std::array<glm::vec4, 2> ColorAmbRegs;
	std::array<glm::vec4, 2> KonstColor;
	std::array<glm::vec4, 4> Color;
	std::array<glm::mat4x3, 10> TexMtx;
	// sizex, sizey, 0, bias
	std::array<glm::vec4, 8> TexParams;
	std::array<glm::mat2x3, 3> IndTexMtx;
	// Optional: Not optional for now.
	Light u_LightParams[8];
};


} // namespace libcube
