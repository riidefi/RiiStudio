#include <llvm/Support/InitLLVM.h>

extern "C" {
struct llvm_InitLLVM;

llvm_InitLLVM* init_llvm(int* argc, const char*** argv,
                         int installPipeSignalExitHandler) {
  auto* p = new llvm::InitLLVM(*argc, *argv, installPipeSignalExitHandler);
  return reinterpret_cast<llvm_InitLLVM*>(p);
}

void deinit_llvm(llvm_InitLLVM* llvm) {
  auto* p = reinterpret_cast<llvm::InitLLVM*>(llvm);
  delete p;
}
}

namespace llvm {
int DisableABIBreakingChecks;
}
