// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "CPUDetect.h"

#if !defined(ARCH_X64)

CPUInfo cpu_info;

CPUInfo::CPUInfo()
{
}

std::string CPUInfo::Summarize()
{
  return "Generic";
}

#endif
