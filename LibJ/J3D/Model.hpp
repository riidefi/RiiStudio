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
#include <LibCore/api/Collection.hpp>


namespace libcube::jsystem {

// Not a FileState for now -- always accessed through a J3DCollection.
// This is subject to change
// No compatibility with old BMD files yet.
struct J3DModel : public px::CollectionHost
{
	~J3DModel() = default;

	PX_TYPE_INFO("J3D Model", "j3d_model", "J::J3DModel");

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

	J3DModel() : px::CollectionHost({ std::string(Material::TypeInfo.namespacedId), std::string(Joint::TypeInfo.namespacedId) }) {}

	template<typename T>
	struct ConcreteCollectionHandle : public CollectionHandle
	{
		ConcreteCollectionHandle(CollectionHandle&& c)
			: CollectionHandle(c.getCollection())
		{
		}

		T& operator[] (std::size_t idx) { return *at<T>(idx); }
		const T& operator[] (std::size_t idx) const { return *at<T>(idx); }
	};

	ConcreteCollectionHandle<Material> getMaterials() { return ConcreteCollectionHandle<Material>(getFolder<Material>().value()); }
	ConcreteCollectionHandle<Material> getMaterials() const { return ConcreteCollectionHandle<Material>(getFolder<Material>().value()); }
	ConcreteCollectionHandle<Joint> getBones() { return ConcreteCollectionHandle<Joint>(getFolder<Joint>().value()); }
	ConcreteCollectionHandle<Joint> getBones() const { return ConcreteCollectionHandle<Joint>(getFolder<Joint>().value()); }


	std::vector<DrawMatrix> mDrawMatrices;
	std::vector<Shape> mShapes;
	std::vector<Texture> mTextures;
};

} // namespace libcube::jsystem
