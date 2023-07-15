#pragma once

namespace riistudio::lvl {

const char* GetNameOfObject(int id);

const char* GetNameOfObjectSafe(int id);

extern unsigned char ObjFlow_bin[];
extern unsigned int ObjFlow_bin_len;

} // namespace riistudio::lvl
