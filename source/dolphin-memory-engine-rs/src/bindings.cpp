#include "bindings.h"

#include "Common/CommonUtils.h"
#include "DolphinAccessor.h"

void DolphinAccessor_init() { DolphinComm::DolphinAccessor::init(); }
void DolphinAccessor_free() { DolphinComm::DolphinAccessor::free(); }
void DolphinAccessor_hook() { DolphinComm::DolphinAccessor::hook(); }
void DolphinAccessor_unHook() { DolphinComm::DolphinAccessor::unHook(); }
int32_t DolphinAccessor_readFromRAM(uint32_t addr, char* buffer, uint32_t size,
                                    int32_t withBSwap) {
#if 0
  printf("DolphinAccessor_readFromRAM: %x, %p, %u, %d\n", addr, (void*)buffer,
         size, withBSwap);
#endif
  u32 offset = Common::dolphinAddrToOffset(addr, false);
  return DolphinComm::DolphinAccessor::readFromRAM(offset, buffer, size,
                                                   withBSwap);
}
int32_t DolphinAccessor_writeToRAM(uint32_t addr, const char* buffer,
                                   uint32_t size, int32_t withBSwap) {
  u32 offset = Common::dolphinAddrToOffset(addr, false);
  return DolphinComm::DolphinAccessor::writeToRAM(offset, buffer, size,
                                                  withBSwap);
}
int32_t DolphinAccessor_getPID() {
  return DolphinComm::DolphinAccessor::getPID();
}
uint64_t DolphinAccessor_getEmuRAMAddressStart() {
  return DolphinComm::DolphinAccessor::getEmuRAMAddressStart();
}
uint32_t DolphinAccessor_getStatus() {
  return static_cast<DolphinStatus>(DolphinComm::DolphinAccessor::getStatus());
}
int32_t DolphinAccessor_isARAMAccessible() {
  return DolphinComm::DolphinAccessor::isARAMAccessible();
}
uint64_t DolphinAccessor_getARAMAddressStart() {
  return DolphinComm::DolphinAccessor::getARAMAddressStart();
}
int32_t DolphinAccessor_isMEM2Present() {
  return DolphinComm::DolphinAccessor::isMEM2Present();
}
char* DolphinAccessor_getRAMCache() {
  return DolphinComm::DolphinAccessor::getRAMCache();
}
uint32_t DolphinAccessor_getRAMCacheSize() {
  return DolphinComm::DolphinAccessor::getRAMCacheSize();
}
int32_t DolphinAccessor_updateRAMCache() {
  return static_cast<int>(DolphinComm::DolphinAccessor::updateRAMCache());
}
#if 0
char* DolphinAccessor_getFormattedValueFromCache(uint32_t ramIndex,
                                                 int32_t memType,
                                                 uint32_t memSize,
                                                 int32_t memBase,
                                                 int32_t memIsUnsigned) {
  return DolphinComm::DolphinAccessor::getFormattedValueFromCache(
      ramIndex, memType, memSize, memBase, memIsUnsigned);
}
#endif
void DolphinAccessor_copyRawMemoryFromCache(char* dest, uint32_t consoleAddress,
                                            uint32_t byteCount) {
  return DolphinComm::DolphinAccessor::copyRawMemoryFromCache(
      dest, consoleAddress, byteCount);
}
int32_t DolphinAccessor_isValidConsoleAddress(uint32_t address) {
  return DolphinComm::DolphinAccessor::isValidConsoleAddress(address);
}
