#include "binary_writer.hxx"

namespace oishii {

Writer::Writer(std::endian endian) : m_endian(endian) {}
Writer::Writer(uint32_t buffer_size, std::endian endian)
    : VectorWriter(buffer_size), m_endian(endian) {}
Writer::Writer(std::vector<u8>&& buf, std::endian endian)
    : VectorWriter(std::move(buf)), m_endian(endian) {}

uint32_t Writer::reserveNext(int32_t n) {
  assert(n > 0);
  if (n == 0)
    return tell();

  const auto start = tell();

  skip(n - 1);
  write<u8>(0, false);
  skip(-n);
  assert(tell() == start);

  return start;
}

void Writer::saveToDisk(std::string_view path) const { FlushFile(mBuf, path); }

void Writer::breakPointProcess(uint32_t tell, uint32_t size) {
  if (shouldBreak(tell, size)) {
    printf("Writing to %04u (0x%04x) sized %u\n", tell, tell, size);
    rsl::debug_break();
  }
}
void Writer::breakPointProcess(uint32_t size) {
  breakPointProcess(tell(), size);
}

} // namespace oishii
