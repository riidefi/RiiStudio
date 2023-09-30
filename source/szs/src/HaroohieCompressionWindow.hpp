// @file HaroohieCompressionWindow.hpp
//
// @author Gericom, (riidefi)
//
// @brief C++ port of HaroohiePals.IO.Compression.CompressionWindow
// https://github.com/HaroohiePals/MarioKartToolbox/blob/main/HaroohiePals.IO/Compression/CompressionWindow.cs
//
#pragma once

#include "Allocator.h"

#include <algorithm>
#include <list>
#include <span>
#include <stdint.h>
#include <vector>

namespace HaroohiePals {

class CompressionWindow {
private:
  std::span<const uint8_t> _src;
  std::list<uint32_t, Moya::Allocator<uint32_t, 16 * 1024>> _windowDict[256];

public:
  uint32_t Position = 0;

  const int MinRun;
  const int MaxRun;
  const int WindowSize;

  CompressionWindow(std::span<const uint8_t> src, int minRun, int maxRun,
                    int windowSize)
      : _src(src), MinRun(minRun), MaxRun(maxRun), WindowSize(windowSize) {
    for (int i = 0; i < 256; ++i)
      _windowDict[i].clear();
  }

  std::pair<uint32_t, int> FindRun() {
    int bestLen = -1;
    uint32_t bestPos = 0;
    for (auto it = _windowDict[_src[Position]].begin();
         it != _windowDict[_src[Position]].end();) {
      uint32_t pos = *it;

      if (Position - pos > WindowSize) {
        it = _windowDict[_src[Position]].erase(it);
        continue;
      }

      int len = 1;
      int lenMax = std::min(static_cast<int>(_src.size() - Position), MaxRun);
      while (_src[pos + len] == _src[Position + len] && len < lenMax)
        ++len;
      if (len >= MinRun && len > bestLen) {
        bestLen = len;
        bestPos = pos;
        if (len == MaxRun)
          break;
      }

      ++it;
    }

    return {bestPos, bestLen};
  }

  void Slide(int count) {
    for (uint32_t i = Position; i < Position + count; ++i)
      _windowDict[_src[i]].push_front(i);
    Position += count;
  }

  void Reset() {
    Position = 0;
    for (int i = 0; i < 256; ++i)
      _windowDict[i].clear();
  }
};

} // namespace HaroohiePals
