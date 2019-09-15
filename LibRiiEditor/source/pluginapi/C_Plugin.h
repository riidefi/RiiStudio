#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.hpp"



/*

Package (Plugin):
	- File formats (detection)
	- File handlers (implement interfaces)

BRRES: Holders -- Model, Texture (Outliner)

*/

// C Api name: RX

#pragma region Helpers
//#define RX_ARRAY_EX(TName, TEntry, TCount) \
//	struct \
//	{ \
//		const TEntry* mpEntries; \
//		const TCount  nEntries;  \
//	}
//#define RX_ARRAY(TEntry) \
//	RX_ARRAY_EX(RK_ARRAY_##TEntry, void*, u32)
typedef struct
{
	const void** mpEntries;
	u32 nEntries;
} RXArray;
#define RX_ARRAY(T) RXArray
typedef const char* zstring;

//	typedef struct {
//		void (*destroy)(RXDestructable* self);
//	} RXDestructable;

typedef struct {
	zstring mName; // human name, "Decode Model"
	zstring mId; // namespaced name "decode_mdl_xform" (implied "package::...")
	zstring mCommandName; // for cli, "decode"
} RXRichName;
#pragma endregion


/*! A file editor handles a file.
	Interfaces:
		- "core::transform_stack": Transformation stack acts as CLI and GUI generator.
			- Implemented here: RXITransformStack
			- getTransforms -> const static array of transforms
*/

typedef struct
{
	zstring mId; // Id of interface, say "core::transform_stack"
	void*	body; // Interface definition, matched by type mId
} RXInterface;

typedef struct
{
	RX_ARRAY(zstring)		mExtensions;
	RX_ARRAY(u32)			mMagics;
	RX_ARRAY(RXInterface)	mInterfaces;
} RXFileEditor;
/*! A package bundles multiple file editors.
*/
typedef struct
{
	RX_ARRAY(RXFileEditor)	mFileEditorRegistrations;
	zstring					mPackageName;		// Human readable name displayed to user
	zstring					mPackageNamespace;	// For internal use
} RXPackage;

#pragma region Transform Stack Interface

enum RXITransformParamType
{
	RXITransformParamType_Flag, // A flag (no argument)
	RXITransformParamType_String, // A simple string
	RXITransformParamType_Enum, // An enumeration of values, object must be of type RXITransformParamEnumOptions

};
typedef struct
{
	RX_ARRAY(RXRichName) mOptions;
	const RXRichName* mDefaultOption; // nullptr if none
} RXITransformParamEnumOptions;

typedef struct
{
	RXRichName mName;
	u32 mType; // of RXITransformParamType
	void* object;
} RXITransformParam;

typedef struct  RXITransform
{
	RXRichName mName;
	// Arguments
	RX_ARRAY(RXITransformParam) mParams;

	void (*mPerform)(void* self);
} RXITransform;

typedef struct
{
	RX_ARRAY(RXITransform) mTransforms;
} RXITransformStack;

#pragma endregion

#ifdef __cplusplus
}
#endif
