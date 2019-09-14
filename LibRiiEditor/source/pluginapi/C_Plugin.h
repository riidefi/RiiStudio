#pragma once

#include "common.hpp"

#pragma region Misc
enum
{
	C_PluginRegistration_Certain, C_PluginRegistration_Maybe, C_PluginRegistration_Unsupported, C_PluginRegistration_Corrupt, C_PluginRegistration_No
};

typedef u32(*C_Plugin_FileInstrusiveCheckingFunction)();
#pragma endregion

//! @brief Constructs a plugin component.
//!
typedef void*(*C_Plugin_Constructor)();

//! @brief Destructs a plugin component.
//!
typedef void(*C_Plugin_Destructor)(void*);


//! @brief The domain of a plugin endpoint. (a certain file)
//!
typedef struct
{
	const char* file_type_name;

	const char** supported_extensions;
	const u32    num_supported_extensions;

	const u32*	 supported_magics;
	const u32    num_supported_magics;

	// !
	C_Plugin_FileInstrusiveCheckingFunction intrusive_check;

	
} C_Plugin_File;
