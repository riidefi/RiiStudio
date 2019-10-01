/// Must include to properly build (tested in windows, vs2019)
//////////////////////////////////////////////////////////////////////
#if _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <future>
#endif
//////////////////////////////////////////////////////////////////////

#include <pfd/portable-file-dialogs.h>
