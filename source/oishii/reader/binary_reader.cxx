#include "binary_reader.hxx"

namespace oishii {

void BinaryReader::readerBpCheck(uint32_t size, s32 trans) {
#ifndef NDEBUG
  if (shouldBreak(tell() + trans, size)) {
    printf("Reading from %04u (0x%04x) sized %u\n", tell, tell, size);
    warnAt("Breakpoint hit", tell() + trans, tell() + trans + size);
    rsl::debug_break();
  }
#endif
}

struct BinaryReader::DispatchStack {
  struct Entry {
    uint32_t jump; // Offset in stream where jumped
    uint32_t jump_sz;

    std::string handlerName; // Name of handler
    uint32_t handlerStart;   // Start address of handler
  };

  std::array<Entry, 16> mStack;
  uint32_t mSize = 0;

  void push_entry(uint32_t j, std::string_view name, uint32_t start = 0) {
    Entry& cur = mStack[mSize];
    ++mSize;

    cur.jump = j;
    cur.handlerName = name;
    cur.handlerStart = start;
    cur.jump_sz = 1;
  }
};

void BinaryReader::enterRegion(std::string&& name, uint32_t& jump_save,
                               uint32_t& jump_size_save, uint32_t start,
                               uint32_t size) {
  if (mStack == nullptr) {
    mStack = std::make_unique<DispatchStack>();
  }

  auto& stack = *this->mStack;

  //_ASSERT(mStack.mSize < 16);
  stack.push_entry(start, name, start);

  // Jump is owned by past block
  if (stack.mSize > 1) {
    jump_save = stack.mStack[stack.mSize - 2].jump;
    jump_size_save = stack.mStack[stack.mSize - 2].jump_sz;
    stack.mStack[stack.mSize - 2].jump = start;
    stack.mStack[stack.mSize - 2].jump_sz = size;
  }
}
void BinaryReader::exitRegion(uint32_t jump_save, uint32_t jump_size_save) {
  assert(mStack != nullptr);
  auto& stack = *this->mStack;
  if (stack.mSize > 1) {
    stack.mStack[stack.mSize - 2].jump = jump_save;
    stack.mStack[stack.mSize - 2].jump_sz = jump_size_save;
  }

  --stack.mSize;
}

//
// This is crappy, POC code -- plan on redoing it entirely
//

void BinaryReader::warnAt(const char* msg, uint32_t selectBegin,
                          uint32_t selectEnd, bool checkStack) {

  if (checkStack) // TODO, unintuitive limitation
  {
    // TODO: Warn class
    fprintf(stderr, "%s:0x%02X: ", getFile(), selectBegin);
    {
      ScopedFormatter fmt(0xe);
      fprintf(stderr, "warning: ");
    }

    fprintf(stderr, "%s\n", msg);
  } else
    fprintf(stderr, "\t\t");
  // Selection
  fprintf(stderr,
          "\tOffset\t00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n\t");

  if (!checkStack)
    fprintf(stderr, "\t\t");

  // We write it at 16 bit lines, a selection may go over multiple lines, so
  // this may not be the best approach
  uint32_t lineBegin = selectBegin / 16;
  uint32_t lineEnd = selectEnd / 16 + !!(selectEnd % 16);

  // Write hex lines
  for (uint32_t i = lineBegin; i < lineEnd; ++i) {
    fprintf(stderr, "%06X\t", i * 16);

    for (int j = 0; j < 16; ++j)
      fprintf(stderr, "%02X ",
              *reinterpret_cast<const u8*>(getStreamStart() + i * 16 + j));

    for (int j = 0; j < 16; ++j) {
      const u8 c = *(getStreamStart() + lineBegin * 16 + j);
      fprintf(stderr, "%c", isprint(c) ? c : '.');
    }

    fprintf(stderr, "\n\t");
    fprintf(stderr, "      \t");
  }

  if (!checkStack)
    fprintf(stderr, "\t\t");

  for (uint32_t i = lineBegin * 16; i < selectBegin; ++i)
    fprintf(stderr, "   ");

  {
    ScopedFormatter fmt(0xa);

    fprintf(stderr, selectEnd - selectBegin == 0
                        ? "^ "
                        : "^~"); // one less, one over below
    for (uint32_t i = selectBegin + 1; i < selectEnd; ++i)
      fprintf(stderr, "~~~");
  }

  for (uint32_t i = selectEnd; i < lineEnd * 16; ++i)
    fprintf(stderr, "   ");

  fprintf(stderr, " ");

  for (uint32_t i = lineBegin * 16; i < selectBegin; ++i)
    fprintf(stderr, " ");

  {
    ScopedFormatter fmt(0xa);

    fprintf(stderr, "^");
    for (uint32_t i = selectBegin + 1; i < selectEnd; ++i)
      fprintf(stderr, "~");
  }
  fprintf(stderr, "\n");

  // Stack trace

  if (checkStack) {
    // printf("\tStack Trace\n\t===========\n");
    if (mStack != nullptr) {
      auto& mStack = *this->mStack;
      for (s32 i = mStack.mSize - 1; i >= 0; --i) {
        const auto& entry = mStack.mStack[i];
        printf("\t\tIn %s: start=0x%X, at=0x%X\n",
               entry.handlerName.size() ? entry.handlerName.c_str() : "?",
               entry.handlerStart, entry.jump);

        if (entry.jump != selectBegin &&
            (i == static_cast<s32>(mStack.mSize) - 1 ||
             mStack.mStack[i + 1].jump != entry.jump))
          warnAt("STACK TRACE", entry.jump, entry.jump + entry.jump_sz, false);
      }
    }
  }
}

BinaryReader::BinaryReader(std::vector<u8>&& view, std::string_view path,
                           std::endian endian)
    : VectorStream(std::move(view)), m_endian(endian), m_path(path) {}
BinaryReader::BinaryReader(std::span<const u8> view, std::string_view path,
                           std::endian endian)
    : VectorStream(std::vector<u8>{view.begin(), view.end()}), m_endian(endian),
      m_path(path) {}
BinaryReader::~BinaryReader() = default;

BinaryReader::BinaryReader(BinaryReader&&) = default;

std::expected<BinaryReader, std::string>
BinaryReader::FromFilePath(std::string_view path, std::endian endian) {
  auto vec = TRY(UtilReadFile(path));
  return BinaryReader(std::move(vec), path, endian);
}

template <typename T, EndianSelect E = EndianSelect::Current,
          bool unaligned = false>
std::expected<T, std::string> tryReadImpl(oishii::BinaryReader& reader) {
  auto result = reader.tryGetAt<T, E, unaligned>(reader.tell());
  if (result.has_value()) {
    // Only advance stream on success
    reader.seekSet(reader.tell() + sizeof(T));
  }
  return result;
}

#define FOR_OISHII_TYPES(F_)                                                   \
  F_(uint8_t)                                                                  \
  F_(int8_t)                                                                   \
  F_(uint16_t)                                                                 \
  F_(int16_t)                                                                  \
  F_(uint32_t)                                                                 \
  F_(int32_t)                                                                  \
  F_(float)

#define TRY_READ_IMPL_TEU(T_, E_, U_)                                          \
  template <>                                                                  \
  std::expected<T_, std::string> BinaryReader::tryRead<T_, E_, U_>() {         \
    return tryReadImpl<T_, E_, U_>(*this);                                     \
  }
#define TRY_READ_IMPL_T(T)                                                     \
  TRY_READ_IMPL_TEU(T, EndianSelect::Current, false)                           \
  TRY_READ_IMPL_TEU(T, EndianSelect::Current, true)                            \
  TRY_READ_IMPL_TEU(T, EndianSelect::Big, false)                               \
  TRY_READ_IMPL_TEU(T, EndianSelect::Big, true)                                \
  TRY_READ_IMPL_TEU(T, EndianSelect::Little, false)                            \
  TRY_READ_IMPL_TEU(T, EndianSelect::Little, true)

FOR_OISHII_TYPES(TRY_READ_IMPL_T)

} // namespace oishii
