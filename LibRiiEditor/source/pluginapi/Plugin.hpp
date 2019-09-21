#pragma once

#include <string>
#include <memory>
#include <memory>

#include "ui/Window.hpp"
#include "core/WindowManager.hpp"

#include <oishii/reader/binary_reader.hxx>
#include "imgui/imgui.h"

namespace pl {

struct RichName
{
	std::string exposedName;
	std::string namespacedId;
	std::string commandName;
};



struct Package;
struct FileEditor;
struct Interface;


struct Package
{
	RichName mPackageName; // Command name unused

	std::vector<std::unique_ptr<FileEditor>> mEditors;
};
enum class InterfaceID
{
	None,

	//! Acts as CLI (and GUI) generator.
	TransformStack,

	//! Files that can be opened (endpoints)
	Readable
};



struct AbstractInterface
{
	AbstractInterface(InterfaceID ID = InterfaceID::None) : mInterfaceId(ID) {}
	virtual ~AbstractInterface() = default;

	InterfaceID mInterfaceId;
};



struct FileEditor
{
	FileEditor() = default;
	// FileEditor(FileEditor&&) = delete;
	FileEditor(const FileEditor& other)
	{
		mExtensions = other.mExtensions;
		mMagics = other.mMagics;
		mInterfaces.reserve(other.mInterfaces.size());
		for (const auto it : other.mInterfaces)
			mInterfaces.push_back(it);
	}

	std::vector<std::string> mExtensions;
	std::vector<u32> mMagics;
	// TODO: These must be part of the child class itself!
	std::vector<AbstractInterface*> mInterfaces;
};

struct Readable : public AbstractInterface
{
	Readable() : AbstractInterface(InterfaceID::Readable) {}
	~Readable() override = default;

	virtual bool tryRead(oishii::BinaryReader& reader, FileEditor& ctx) = 0;
};

// Transform stack
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



}
