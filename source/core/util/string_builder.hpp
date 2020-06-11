#pragma once

#include <algorithm> // for std::fill
#include <core/common.h> // for assert
#include <string_view> // for std::string_view

namespace riistudio::util {

class StringBuilder {
public:
  StringBuilder(char* buf, std::size_t size)
      : mBuf(buf), mIt(buf), mEnd(buf + size) {
    std::fill(mBuf, mEnd, '\0');
  }

  void append(std::string_view string) {
    assert(mIt + string.length() < mEnd);
    std::memcpy(mIt, string.data(), string.length());
    mIt += string.length();
  }
  void reset() {
    std::fill(mBuf, mIt, '\0');
    mIt = mBuf;
  }

private:
  char* mBuf;
  char* mIt;
  char* mEnd;
};

} // namespace riistudio::util
