#pragma once

#include <map>
#include <string>
#include <vector>

#include <LibCube/GX/VertexTypes.hpp>
#include <ThirdParty/glm/mat4x4.hpp>
#include <LibCube/Util/BoundBox.hpp>
#include <oishii/writer/binary_writer.hxx>
#include <LibJ/J3D/Joint.hpp>
#include <LibJ/J3D/DrawMatrix.hpp>
#include <LibJ/J3D/Material.hpp>
#include <LibJ/J3D/VertexBuffer.hpp>
#include <LibJ/J3D/Shape.hpp>
#include "Texture.hpp"

#include <LibCore/api/Node.hpp>


namespace libcube::jsystem {

// Not a FileState for now -- always accessed through a J3DCollection.
// This is subject to change
// No compatibility with old BMD files yet.
struct J3DModel : public px::IDestructable
{
	~J3DModel() = default;

	PX_TYPE_INFO_EX("J3D Model", "j3d_model", "J::J3DModel", ICON_FA_CUBES, ICON_FA_CUBE);

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

		ScalingRule mScalingRule = ScalingRule::Basic;
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

	J3DModel()
	{
		acceptChild(PX_GET_TID(Material));
		acceptChild(PX_GET_TID(Joint));
		acceptChild(PX_GET_TID(Shape));
	}

	PX_COLLECTION_FOLDER_GETTER(getMaterials, Material);
	PX_COLLECTION_FOLDER_GETTER(getBones, Joint);
	PX_COLLECTION_FOLDER_GETTER(getShapes, Shape);

	std::vector<DrawMatrix> mDrawMatrices;
};

} // namespace libcube::jsystem
