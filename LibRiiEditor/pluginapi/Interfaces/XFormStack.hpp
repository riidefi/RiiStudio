#pragma once

#include <vector>
#include "LibRiiEditor/pluginapi/RichName.hpp"
#include "LibRiiEditor/pluginapi/Interface.hpp"

namespace pl {

struct TransformStack : public AbstractInterface
{
	TransformStack() : AbstractInterface(InterfaceID::TransformStack) {}
	~TransformStack() override = default;

	enum class ParamType
	{
		Flag, // No arguments
		String, // Simple string
		Enum, // Enumeration of values
	};

	struct EnumOptions
	{
		std::vector<RichName> enumeration;
		int mDefaultOptionIndex;
	};

	struct Param
	{
		RichName name;
		ParamType type;

		// Only for enum
		EnumOptions enum_opts;
	};

	struct XForm
	{
		RichName name;
		std::vector<Param> mParams;
		virtual void perform() {}
		virtual ~XForm() = default;
	};

	// Must exist inside this object -- never freed!
	std::vector<XForm*> mStack;
};

} // namespace pl
