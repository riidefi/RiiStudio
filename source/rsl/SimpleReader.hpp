#pragma once

#include <llvm/Support/Endian.h>
#include <span>
#include <stdint.h>

namespace rsl {

using byte_view = std::span<const uint8_t>;

//! Unsafe API: Verify the operation before calling
template <typename T> static T load(byte_view data, unsigned offset) {
  assert(offset + sizeof(T) <= data.size_bytes());

  return llvm::sys::getSwappedBytes(
      *reinterpret_cast<const T*>(data.data() + offset));
}

//! Unsafe API: Verify the operation before calling
//! (Writeback form)
template <typename T> static T load_update(byte_view data, unsigned& offset) {
  const T result = load<T>(data, offset);
  offset += sizeof(T);
  return result;
}

namespace pp {

float_t lfs(byte_view data, unsigned offset) {
  return load<float_t>(data, offset);
}
uint32_t lwz(byte_view data, unsigned offset) {
  return load<uint32_t>(data, offset);
}
uint16_t lhz(byte_view data, unsigned offset) {
  return load<uint16_t>(data, offset);
}
int16_t lha(byte_view data, unsigned offset) {
  return load<int16_t>(data, offset);
}
uint8_t lbz(byte_view data, unsigned offset) {
  return load<uint8_t>(data, offset);
}

float lfsu(byte_view data, unsigned& offset) {
  return load_update<float>(data, offset);
}
uint32_t lwzu(byte_view data, unsigned& offset) {
  return load_update<uint32_t>(data, offset);
}
uint16_t lhzu(byte_view data, unsigned& offset) {
  return load_update<uint16_t>(data, offset);
}
int16_t lhau(byte_view data, unsigned& offset) {
  return load_update<int16_t>(data, offset);
}
uint32_t lbzu(byte_view data, unsigned& offset) {
  return load_update<uint8_t>(data, offset);
}

} // namespace pp

} // namespace rsl
