
#include <Windows.h>
#include <LibRiiEditor/core/PluginFactory.hpp>

using riimain_fn_t = void (CALLBACK*) (PluginFactory*);

bool PluginFactory::installModule(const std::string& path)
{
	auto hnd = LoadLibraryA(path.c_str());

	if (!hnd)
		return false;

	typedef HRESULT(CALLBACK* LPFNDLLFUNC1)(DWORD, UINT*);
	
	auto fn = reinterpret_cast<riimain_fn_t>(GetProcAddress(hnd, "__riimain"));
	if (!fn)
		return false;

	fn(this);
	return true;
}
