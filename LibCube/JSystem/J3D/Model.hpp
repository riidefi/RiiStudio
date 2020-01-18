#pragma once

#include <map>
#include <string>
#include <vector>

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>
#include <LibCube/GX/VertexTypes.hpp>
#include <ThirdParty/glm/mat4x4.hpp>
#include <LibCube/Common/BoundBox.hpp>
#include <oishii/writer/binary_writer.hxx>
#include <LibCube/JSystem/J3D/Joint.hpp>
#include <LibCube/JSystem/J3D/DrawMatrix.hpp>
#include <LibCube/JSystem/J3D/Material.hpp>
#include <LibCube/JSystem/J3D/VertexBuffer.hpp>
#include <LibCube/JSystem/J3D/Shape.hpp>



namespace libcube::jsystem {

// Not a FileState for now -- always accessed through a J3DCollection.
// This is subject to change
// No compatibility with old BMD files yet.
struct J3DModel
{
	J3DModel() = default;
	~J3DModel() = default;

	template<typename T>
	using ID = int;

	struct Information
	{
		// For texmatrix calculations
		enum class ScalingRule
		{
			Basic,
			XSI,
			Maya
		};

		ScalingRule mScalingRule;
		// nPacket, nVtx

		// Hierarchy data is included in joints.
	};

	Information info;


	struct Bufs
	{
		// FIXME: Good default values
		VertexBuffer<glm::vec3, VBufferKind::position> pos { VQuantization() };
		VertexBuffer<glm::vec3, VBufferKind::normal> norm { VQuantization() };
		std::array<VertexBuffer<gx::Color, VBufferKind::color>, 2> color;
		std::array<VertexBuffer<glm::vec2, VBufferKind::textureCoordinate>, 8> uv;

		Bufs() {}
	} mBufs = Bufs();

	

	std::vector<DrawMatrix> mDrawMatrices;
	std::vector<Joint> mJoints;
	std::vector<Material> mMaterials;
	std::vector<Shape> mShapes;
};

} // namespace libcube::jsystem
