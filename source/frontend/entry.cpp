#include "root.hpp"

struct RootHolder {
  void create() {
    window = std::make_unique<riistudio::frontend::RootWindow>();
  }
  void enter() { window->enter(); }

private:
  std::unique_ptr<riistudio::frontend::RootWindow> window;
} sRootHolder;

#ifdef _WIN32
int main(int argc, char* const* argv)
#else
int main(int, char**)
#endif
{
  sRootHolder.create();
  sRootHolder.enter();

  return 0;
}
