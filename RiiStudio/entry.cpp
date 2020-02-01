#include <RiiStudio/root/RootWindow.hpp>

int main(int argc, char* const* argv)
{
	auto core = std::make_unique<RootWindow>();

	core->loop();

	return 0;
}
