#pragma once

#include <string>
#include <memory>
#include <memory>

#include "ui/Window.hpp"
#include "core/WindowManager.hpp"


namespace pl {

struct RichName
{
	const std::string exposedName;
	const std::string namespacedId;
	const std::string commandName;
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
	TransformStack
};
struct AbstractInterface
{
	AbstractInterface(InterfaceID ID = InterfaceID::None) : mInterfaceId(ID) {}
	virtual ~AbstractInterface() = default;

	InterfaceID mInterfaceId;
};
struct FileEditor
{
	// FileEditor(FileEditor&&) = delete;
	FileEditor(const FileEditor& other)
	{
		mExtensions = other.mExtensions;
		mMagics = other.mMagics;
		mInterfaces.resize(other.mInterfaces.size());
		for (const auto& it : other.mInterfaces)
			mInterfaces.push_back(std::make_unique<AbstractInterface>(*it));
	}

	std::vector<std::string> mExtensions;
	std::vector<u32> mMagics;
	std::vector<std::unique_ptr<AbstractInterface>> mInterfaces;
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
	};

	std::vector<std::unique_ptr<XForm>> mStack; 
};

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(const FileEditor& registration)
		: mEditor(registration)
	{}

	void draw(WindowContext* ctx) noexcept override final
	{
		if (!ctx)
			return;

		//	PluginContext pl_ctx{ *ctx, *static_cast<Window*>(this), *static_cast<WindowManager*>(this) };
		//	getPlugin().plugin_draw(&getPlugin(), &pl_ctx);

		// Check for IRenderable

		// Fill in top bar, check for IExtendedBar

		// Draw childen
		drawWindows();
	}

	const FileEditor& mEditor;
};

}
