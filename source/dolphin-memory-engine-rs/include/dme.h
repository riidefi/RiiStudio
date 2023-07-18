#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  Hooked = 0,
  NotRunning = 1,
  NoEmu = 2,
  UnHooked = 3,
} DolphinStatus_C;

typedef struct DolphinAccessor_C DolphinAccessor_C;

DolphinAccessor_C* dolphin_accessor_new(void);
void dolphin_accessor_drop(DolphinAccessor_C* dolphin_accessor);
void dolphin_accessor_hook(DolphinAccessor_C* dolphin_accessor);
void dolphin_accessor_unhook(DolphinAccessor_C* dolphin_accessor);
int dolphin_accessor_read_from_ram(DolphinAccessor_C* dolphin_accessor,
                                   unsigned int offset, unsigned char* buffer,
                                   int buffer_length, int with_b_swap);
int dolphin_accessor_write_to_ram(DolphinAccessor_C* dolphin_accessor,
                                  unsigned int offset,
                                  const unsigned char* buffer,
                                  int buffer_length, int with_b_swap);
#if 0
int dolphin_accessor_get_pid(DolphinAccessor_C* dolphin_accessor);
unsigned long long
dolphin_accessor_get_emu_ram_address_start(DolphinAccessor_C* dolphin_accessor);
#endif
DolphinStatus_C
dolphin_accessor_get_status(DolphinAccessor_C* dolphin_accessor);
#if 0
int dolphin_accessor_is_aram_accessible(DolphinAccessor_C* dolphin_accessor);
unsigned long long
dolphin_accessor_get_aram_address_start(DolphinAccessor_C* dolphin_accessor);
int dolphin_accessor_is_mem2_present(DolphinAccessor_C* dolphin_accessor);
int dolphin_accessor_is_valid_console_address(
    DolphinAccessor_C* dolphin_accessor, unsigned int address);
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <span>

class DolphinAccessor {
public:
  enum class Status {
    Hooked,
    NotRunning,
    NoEmu,
    UnHooked,
  };

  DolphinAccessor() : m_dolphin_accessor{dolphin_accessor_new()} {}
  DolphinAccessor(const DolphinAccessor&) = delete;
  DolphinAccessor(DolphinAccessor&& rhs) {
    if (m_dolphin_accessor) {
      dolphin_accessor_drop(m_dolphin_accessor);
    }
    m_dolphin_accessor = rhs.m_dolphin_accessor;
    rhs.m_dolphin_accessor = nullptr;
  }
  ~DolphinAccessor() {
    if (m_dolphin_accessor) {
      dolphin_accessor_drop(m_dolphin_accessor);
    }
  }

  void hook() { dolphin_accessor_hook(m_dolphin_accessor); }

  void unhook() { dolphin_accessor_unhook(m_dolphin_accessor); }

  bool readFromRAM(unsigned int offset, std::span<unsigned char> buffer,
                   bool with_b_swap) {
    return dolphin_accessor_read_from_ram(
        m_dolphin_accessor, offset, buffer.data(), buffer.size(), with_b_swap);
  }

  bool writeToRAM(unsigned int offset, std::span<const unsigned char> buffer,
                  bool with_b_swap) {
    return dolphin_accessor_write_to_ram(
        m_dolphin_accessor, offset, buffer.data(), buffer.size(), with_b_swap);
  }

  Status getStatus() {
    return static_cast<Status>(dolphin_accessor_get_status(m_dolphin_accessor));
  }

private:
  DolphinAccessor_C* m_dolphin_accessor = nullptr;
};
#endif
