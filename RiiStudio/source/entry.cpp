#include "core/RiiCore.hpp"

int main()
{
	auto core = std::make_unique<RiiCore>();

	core->frameLoop();

	return 0;
}
