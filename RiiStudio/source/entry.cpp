#include "core/RiiCore.hpp"

int main(int argc, char* const* argv)
{
	auto core = std::make_unique<RiiCore>();

	core->frameLoop();

	return 0;
}
