#pragma once

namespace pl {

enum class InterfaceID
{
	None,

	//! Acts as CLI (and GUI) generator.
	TransformStack,

	TextureList,

	LibCube_GCCollection
};


struct AbstractInterface
{
	AbstractInterface(InterfaceID ID = InterfaceID::None) : mInterfaceId(ID) {}
	virtual ~AbstractInterface() = default;

	InterfaceID mInterfaceId;
};

} // namespace pl
