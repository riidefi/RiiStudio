#pragma once

#include <string>
#include "LibRiiEditor/pluginapi/Interface.hpp"
#include <oishii/types.hxx>

namespace pl {

struct ITextureList : public AbstractInterface
{
	ITextureList() : AbstractInterface(InterfaceID::TextureList) {}
	~ITextureList() override = default;

	virtual u32 getNumTex() const = 0;
	virtual std::string getNameAt(int idx) const = 0;
};

} // namespace pl
