#include "C_Plugin.h"

// Demo plugin name: CDemoPlugin, cdemofile, etc

const RXFileEditor CDemoFile1Editor;

const RXPackage CDemoPluginPackage = {
	// mFileEditorRegistrations
	{
		&CDemoFile1Editor,
		1
	},
	// mPackageName
	"C Demo Plugin",
	// mPackageNamespace
	"cdemoplugin"
};

const char* demoFileExts[1] = { ".cdemofile" };
const u32 demoFileMagics[1] = { 'CDM0'};

void demoFileBeepPerform(RXITransform* self)
{
	printf("Boop!\n");
}

const RXITransform demoFileXforms[1] = {
	{
		{
			"Beep!",
			"demo_xform_beep",
			"beep"
		},
		{
			NULL,
			0
		},
		demoFileBeepPerform
	}
};
static const RXITransformStack demoFileXformStack = { { demoFileXforms, 1}};

static const RXInterface demoFileInterfaces[1] = { {"core::transform_stack", &demoFileXformStack} };

const RXFileEditor CDemoFile1Editor = {
	// mExtensions
	{
		&demoFileExts[0],
		1
	},
	// mMagics
	{
		&demoFileMagics[0],
		1
	},
	// mInterfaces
	{
		&demoFileInterfaces[0],
		1
	}
};
