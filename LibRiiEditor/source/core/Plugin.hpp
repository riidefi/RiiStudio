#pragma once

#include "common.hpp"

#ifdef __cplusplus
#include "WindowManager.hpp"

#include <string>
#include <memory>

struct PluginFactory;
struct PluginContext
{
	WindowContext window;
	WindowManager& window_manager;
};
//! @brief	A single applet may have multiple plugins. A plugin editor is a window of the root.
//!			An applet's core resource will reflect the state of the program; a plugin core resource will reflect the data being edited.
//!
struct Plugin
{
	//! Set by manager after plugin is constructed.
	//!	Ensured to be valid for the lifetime of the plugin.
	//!
	PluginFactory* mpPluginFactory;

	typedef void(*PluginDraw)(PluginContext*);

	PluginDraw plugin_draw;
};




//! @brief Kept C-like for easier support across ABIs and languages.
//!
struct PluginRegistration
{
	template<typename T>
	struct PluginSpan
	{
		const T* data;
		u32		 size;
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

//! @brief A plugin factory must also destroy what it creates -- not necessarily same heap in main program.
//!
class PluginInstance
{
public:
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

	Plugin& getPlugin() noexcept
	{
		assert(mpPlugin);
		return *mpPlugin;
	}
	const Plugin& getPlugin() const noexcept
	{
		assert(mpPlugin);
		return *mpPlugin;
	}
	const PluginRegistration& getRegistration() const noexcept
	{
		return registration;
	}

private:
	Plugin *const mpPlugin;
	const PluginRegistration& registration;
};


struct PluginWindow : public PluginInstance, public WindowManager, public Window
{
	~PluginWindow() override = default;
	PluginWindow(const PluginRegistration& registration)
		: PluginInstance(registration)
	{}

	void draw(WindowContext* ctx) noexcept override final
	{
		if (!ctx)
			return;

		PluginContext pl_ctx{ *ctx, *static_cast<WindowManager*>(this) };
		getPlugin().plugin_draw(&pl_ctx);
	}

};

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

typedef struct C_PluginContext
{
	void* selectionManager;
	void* coreResource;
	void* windowManager;
} C_PluginContext;

typedef u32 C_PluginRegistration_CheckResult;
typedef u32(*C_PluginRegistration_instrusiveCheckingFunction)();
typedef void*(*C_PluginRegistration_pluginConstructor)();
typedef void(*C_PluginRegistration_pluginDestructor)(void*);

enum
{
	C_PluginRegistration_Certain, C_PluginRegistration_Maybe, C_PluginRegistration_Unsupported, C_PluginRegistration_Corrupt, C_PluginRegistration_No
};
typedef struct C_PluginRegistration_
{
	const char** supported_extensions;
	const u32    num_supported_extensions;

	const u32*	 supported_magics;
	const u32    num_supported_magics;

	C_PluginRegistration_instrusiveCheckingFunction intrusive_check;

	const char* plugin_name;
	const char* plugin_version;
	const char* plugin_domain;

	// Plugin itself is not C compatible -- WindowManager component may be separated later to enable full compatibility.
	C_PluginRegistration_pluginConstructor construct;
	C_PluginRegistration_pluginDestructor destruct;
} C_PluginRegistration;

typedef struct
{
	void* factory;
	void* drawFunc;
} C_Plugin;
#ifdef __cplusplus
}
#endif
