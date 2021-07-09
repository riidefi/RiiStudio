#pragma once

#include <core/common.h>
#include <string>

namespace riistudio {

// Call from main derived code. This is to ensure a locale api isn't called
// during static initialization
void MarkLocaleAPIReady();

// Get/Set the locale string: "English" or "Japanese"
void SetLocale(std::string s);
std::string GetLocale();

// Dump to file for development
void DebugDumpLocaleMisses();
// Reload from .csv
void DebugReloadLocales();

} // namespace riistudio
