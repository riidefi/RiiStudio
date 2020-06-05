#include "root.hpp"

#ifdef _WIN32
int main(int argc, char* const* argv)
#else
int main(int, char**)
#endif
{
  DebugReport("Starting...\n");

  auto* core = new riistudio::frontend::RootWindow();

  core->enter();

#ifdef _WIN32
  delete core;
#endif

  return 0;
}
