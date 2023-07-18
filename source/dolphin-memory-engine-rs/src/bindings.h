#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DolphinStatus_hooked,
  DolphinStatus_notRunning,
  DolphinStatus_noEmu,
  DolphinStatus_unHooked
} DolphinStatus;

void DolphinAccessor_init();
void DolphinAccessor_free();
void DolphinAccessor_hook();
void DolphinAccessor_unHook();
int32_t DolphinAccessor_readFromRAM(uint32_t offset, char* buffer,
                                    uint32_t size, int32_t withBSwap);
int32_t DolphinAccessor_writeToRAM(uint32_t offset, const char* buffer,
                                   uint32_t size, int32_t withBSwap);
int32_t DolphinAccessor_getPID();
uint64_t DolphinAccessor_getEmuRAMAddressStart();
DolphinStatus DolphinAccessor_getStatus();
int32_t DolphinAccessor_isARAMAccessible();
uint64_t DolphinAccessor_getARAMAddressStart();
int32_t DolphinAccessor_isMEM2Present();
char* DolphinAccessor_getRAMCache();
uint32_t DolphinAccessor_getRAMCacheSize();
int32_t DolphinAccessor_updateRAMCache();
#if 0
char* DolphinAccessor_getFormattedValueFromCache(uint32_t ramIndex,
                                                 int32_t memType,
                                                 uint32_t memSize,
                                                 int32_t memBase,
                                                 int32_t memIsUnsigned);
#endif
void DolphinAccessor_copyRawMemoryFromCache(char* dest, uint32_t consoleAddress,
                                            uint32_t byteCount);
int32_t DolphinAccessor_isValidConsoleAddress(uint32_t address);

#ifdef __cplusplus
}
#endif
