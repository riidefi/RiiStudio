#pragma once

#include <vendor/llvm/Support/InitLLVM.h>

namespace rsl {

struct llvm_InitLLVM;

inline llvm_InitLLVM* init_llvm(int& argc, const char**& argv,
                                bool installPipeSignalExitHandler = true) {
  auto* p = new llvm::InitLLVM(argc, argv, installPipeSignalExitHandler);
  return reinterpret_cast<llvm_InitLLVM*>(p);
}

inline void deinit_llvm(llvm_InitLLVM* llvm) {
  auto* p = reinterpret_cast<llvm::InitLLVM*>(llvm);
  delete p;
}

class InitLLVM {
public:
  InitLLVM(int& argc, const char**& argv,
           bool installPipeSignalExitHandler = true)
      : m_llvm(init_llvm(argc, argv, installPipeSignalExitHandler)) {}

private:
  using Deleter = decltype([](llvm_InitLLVM* llvm) { deinit_llvm(llvm); });
  std::unique_ptr<llvm_InitLLVM, Deleter> m_llvm;
};

} // namespace rsl
