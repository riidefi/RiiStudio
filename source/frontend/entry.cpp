#include "root.hpp"
#include <memory>
#include <vendor/llvm/Support/InitLLVM.h>

struct RootHolder {
  void create(int& argc, const char**& argv) {
    window = std::make_unique<riistudio::frontend::RootWindow>();
#ifndef BUILD_DEBUG
    initLlvm = std::make_unique<llvm::InitLLVM>(argc, argv);
#endif
  }
  void setForceUpdate(bool update) { window->setForceUpdate(update); }
  void enter() { window->enter(); }

  riistudio::frontend::RootWindow& getWindow() {
    assert(window);
    return *window;
  }

private:
  std::unique_ptr<riistudio::frontend::RootWindow> window;
  std::unique_ptr<llvm::InitLLVM> initLlvm;
} sRootHolder;


int main(int argc, const char** argv) {
  const bool force_update = argc > 1 && !strcmp(argv[1], "update");

  sRootHolder.create(argc, argv);
  sRootHolder.setForceUpdate(force_update);
  sRootHolder.enter();

  return 0;
}
