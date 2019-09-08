#pragma once

#include "common.hpp"
#include "WindowManager.hpp"

#include <string>
#include <memory>

//! @brief	A single applet may have multiple plugins. A plugin editor is a window of the root.
//!			An applet's core resource will reflect the state of the program; a plugin core resource will reflect the data being edited.
//!
struct Plugin : public WindowManager, public Window
{
};

struct PluginRegistration;

//! @brief A plugin factory must also destroy what it creates -- not necessarily same heap in main program.
//!
struct PluginInstance
{
	Plugin *const mpPlugin;
	const PluginRegistration& registration;

	PluginInstance(const PluginRegistration& regist)
		: mpPlugin(regist.construct()), registration(regist)
	{
		assert(mpPlugin);
	}
	~PluginInstance()
	{
		assert(mpPlugin);
		registration.destruct(mpPlugin);
	}
};

//! @brief Kept C-like for easier support across ABIs and languages.
//!
struct PluginRegistration
{
	template<typename T>
	struct PluginSpan
	{
		T*	data;
		u32 size;
	};

	const PluginSpan<const char*>	supported_extensions;
	const PluginSpan<u32>			supported_magics;

	// Above data may be coagulated into tables and allow for efficient searching and identifying duplicate claims ahead of time.
	// Below, a more intensive check is performed. It will be called to confirm a prior match or to create match candidates when uncertainty arises.
	// An intensive check should not verify extensions or magic.
	// Example checks: Files without magics
	enum class CheckResult : u32
	{
		Certain,	 //!< Will give priority when descriminating candidates.
		Maybe,		 //!< Cannot determine -- rarely applicable.
		Unsupported, //!< E.g. invalid file version.
		Corrupt,	 //!< Cannot properly read, but can identify the file.
		No			 //!< Disqualify this plugin when matching files. Prefer Corrupt or Unsupported when identified.
	};
	typedef CheckResult(*instrusiveCheckingFunction)();

	instrusiveCheckingFunction instrusive_check;

	// Plugin information
	const char* plugin_name;
	const char* plugin_version;
	const char* plugin_domain; //!< (where does this plugin apply?)

	// Plugin instancing
	typedef Plugin*(*pluginFactory)();
	pluginFactory construct;

	typedef void(*pluginDestructor)(Plugin*);
	pluginDestructor destruct;
};

extern "C" {
	typedef struct C_PluginRegistration
	{
		const char** supported_extensions;
		const u32    num_supported_extensions;

		const u32*	 supported_magics;
		const u32    num_supported_magics;

		enum CheckResult_
		{
			Certain, Maybe, Unsupported, Corrupt, No
		};

		typedef u32 CheckResult;
		typedef CheckResult(*instrusiveCheckingFunction)();

		instrusiveCheckingFunction intrusive_check;

		const char* plugin_name;
		const char* plugin_version;
		const char* plugin_domain;

		// Plugin itself is not C compatible -- WindowManager component may be separated later to enable full compatibility.
		typedef void*(*pluginFactory)();
		pluginFactory construct;

		typedef void(*pluginDestructor)(void*);
		pluginDestructor destruct;
	} C_PluginRegistration;
}
