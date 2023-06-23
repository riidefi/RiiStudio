struct llvm_InitLLVM;

struct llvm_InitLLVM* init_llvm(int* argc, const char*** argv,
                                int installPipeSignalExitHandler);

void deinit_llvm(struct llvm_InitLLVM* llvm);
