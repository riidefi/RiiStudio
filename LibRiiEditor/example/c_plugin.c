#if 0
#include "core/Plugin.hpp"

// Demo for the C plugin interface.

const char* demo_supportedExtensions[1] = { ".rtest" };
const u32 demo_supportedMagics[] = { 'RTST' };

C_PluginRegistration* get_demo_registration();

u32 demo_intrusiveCheck();
void* demo_construct();
void demo_destruct(void* pl);

static C_PluginRegistration demo_regist = {
	demo_supportedExtensions, 1,
	demo_supportedMagics, 1,
	demo_intrusiveCheck,
	"Test",
	"1.0",
	".rtest Test files",
	demo_construct,
	demo_destruct
};

typedef struct
{
	C_Plugin base;
} DemoPlugin;

void demo_draw(struct C_PluginContext* ctx)
{

}

void* demo_construct()
{
	DemoPlugin* plugin = (DemoPlugin*)malloc(sizeof(DemoPlugin));

	plugin->base.drawFunc = demo_draw;

	return (void*)plugin;
}

void demo_destruct(void* pl)
{
	free(pl);
}



C_PluginRegistration* get_demo_registration()
{
	return &demo_regist;
}

u32 demo_intrusiveCheck()
{
	return C_PluginRegistration_Maybe;
}

#endif
