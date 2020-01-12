#include "core.hpp"

int main(int argc, char* const* argv)
{
#if DEBUG
	static char* const args[] = { "tool.exe", "test.bmd", "--tristrip", "-cache_size=42", "BRRES", "-h=3" };
	rs_cli::System sys(6, &args[0]);
#else
	rs_cli::System sys(argc, argv);
#endif
	return 0;
}
