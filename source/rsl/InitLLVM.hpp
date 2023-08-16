#pragma once

namespace rsl {

// Disabled on emscripten.
// Rationale: browser already handles stacktraces for us.
#ifndef __EMSCRIPTEN__

struct llvm_InitLLVM;

// Links against riidefi/llvm_sighandler crate
extern "C" llvm_InitLLVM*
rsl_init_llvm(int& argc, const char**& argv,
              int installPipeSignalExitHandler = true);

extern "C" void rsl_deinit_llvm(llvm_InitLLVM* llvm);

class InitLLVM {
public:
  InitLLVM(int& argc, const char**& argv,
           bool installPipeSignalExitHandler = true)
      : m_llvm(rsl_init_llvm(argc, argv, installPipeSignalExitHandler)) {}

private:
  using Deleter = decltype([](llvm_InitLLVM* llvm) { rsl_deinit_llvm(llvm); });
  std::unique_ptr<llvm_InitLLVM, Deleter> m_llvm;
};

#else

class InitLLVM {
public:
  InitLLVM(int& argc, const char**& argv,
           bool installPipeSignalExitHandler = true) {}
};

#endif

} // namespace rsl
