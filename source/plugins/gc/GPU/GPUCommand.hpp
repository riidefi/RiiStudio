#pragma once

#include <core/common.h>
#include <vendor/dolemu/BitField.hpp>
// The order and contents that a display list *should* have.
enum class CommandType {
  BP = 0x61, // 1 BP command: 5 bytes
  CP = 0x08,
  XF = 0x10,
  NOP = 0x0,

  // Not a real GPU command: terminator in context layout
  RET = 0xFF
};

union BPCommand {
  // BitField<0, 8, u32>	 mRegister;
  // BitField<8, 32, u32> mValue;
  //
  // u32 hex;
  struct {
    u8 mRegister;
    u8 mValue[3];
  };
  u32 hex;

  BPCommand() : hex(0) {}
  BPCommand(u8 reg, u8* val) : mRegister(reg) {
    // assert(val);
    mValue[0] = val[0];
    mValue[1] = val[1];
    mValue[2] = val[2];
  }
  BPCommand(u32 regval) : hex(regval) {}
};
struct CPCommand {
  u8 mRegister;
  u32 mValue;
};
struct XFCommand {
  u16 nCommands; // There are nCommands + 1 commands, and at most 16 values can
                 // be sent. This seems like it is really, dataEndOffset
  u16 mRegister;
  u32 mValue[1];
};
