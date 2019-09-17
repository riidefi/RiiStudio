#pragma once

#include "C_Plugin.h"
#include <string>

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
	
	std::vector<FileEditor> mEditors;
};

struct FileEditor : public RXFileEditor
{
	std::vector<std::string> mExtensions;
	std::vector<u32> mMagics;
	std::vector<std::unique_ptr<AbstractInterface>> mInterfaces;
};
enum class InterfaceID
{
	None,

	//! Acts as CLI (and GUI) generator.
	TransformStack
};
struct AbstractInterface
{
	AbstractInterface() : mInterfaceId(InterfaceID::None){}
	virtual ~AbstractInterface() = default;

	InterfaceID mInterfaceId;
};

// Transform stack
struct TransformStack : public AbstractInterface
{
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
