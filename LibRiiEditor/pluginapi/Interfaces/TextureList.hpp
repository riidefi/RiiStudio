#pragma once

#include <string>
#include "LibRiiEditor/pluginapi/Interface.hpp"
#include <oishii/types.hxx>

namespace pl {

struct ITextureList
{
	ITextureList() = default;
	virtual ~ITextureList() = default;

	virtual u32 getNumTex() const = 0;
	virtual std::string getNameAt(int idx) const = 0;
};

} // namespace pl
