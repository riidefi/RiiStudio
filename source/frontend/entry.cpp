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
  void enter() { window->enter(); }

private:
  std::unique_ptr<riistudio::frontend::RootWindow> window;
  std::unique_ptr<llvm::InitLLVM> initLlvm;
} sRootHolder;

int main(int argc, const char** argv) {
  sRootHolder.create(argc, argv);
  sRootHolder.enter();

  return 0;
}
