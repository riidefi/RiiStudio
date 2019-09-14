#pragma once

#include "C_Plugin.h"
#include <string>

#include "ui/Window.hpp"
#include "core/WindowManager.hpp"


namespace pl {

struct Package;
struct FileEditor;
struct Interface;

template<typename T>
struct Span : public RXArray
{
	u32 getNum() const
	{
		return nEntries;
	}
	const T& at(u32 idx) const
	{
		assert(idx < getNum());

		return *(T*)(&mpEntries[idx]);
	}
	const T& operator[](u32 idx) const
	{
		return at(idx);
	}
};

struct Package : RXPackage
{
	std::string getExposedName()
	{
		return mPackageName;
	}
	std::string getNamespace()
	{
		return mPackageNamespace;
	}
	const Span<FileEditor>& getEditors() const
	{
		return *(Span<FileEditor>*)&mFileEditorRegistrations;
	}
};

struct FileEditor : public RXFileEditor
{
	const Span<const char*>& getExtensions() const
	{
		return *(Span<const char*>*)&mExtensions;
	}

	const Span<u32>& getMagics() const
	{
		return *(Span<u32>*)&mMagics;
	}

	const Span<Interface>& getInterfaces() const
	{
		return *(Span<Interface>*)&mInterfaces;
	}
};

struct Interface : RXInterface
{
	std::string getId()
	{
		return mId;
	}
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

// Above all for reading, TODO: Introduce wrapper for writing

}
