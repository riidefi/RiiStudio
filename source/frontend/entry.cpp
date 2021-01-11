#include "root.hpp"
#include <memory>
#include <vendor/llvm/Support/InitLLVM.h>

struct RootHolder {
  void create(int& argc, const char**& argv) {
#ifndef BUILD_DEBUG
    initLlvm = std::make_unique<llvm::InitLLVM>(argc, argv);
#endif

    window = std::make_unique<riistudio::frontend::RootWindow>();
  }

  void enter() { window->enter(); }

  riistudio::frontend::RootWindow& getRoot() {
    assert(window);
    return *window;
  }

private:
  std::unique_ptr<riistudio::frontend::RootWindow> window;
  std::unique_ptr<llvm::InitLLVM> initLlvm;
} sRootHolder;

int main(int argc, const char** argv) {
  sRootHolder.create(argc, argv);

  if (argc > 1) {
    if (!strcmp(argv[1], "--update")) {
      sRootHolder.getRoot().setForceUpdate(true);
    } else {
      sRootHolder.getRoot().openFile(argv[1]);
    }
  }

  sRootHolder.enter();

  return 0;
}
