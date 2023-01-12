#include "binary_reader.hxx"

namespace oishii {

void BinaryReader::boundsCheck(u32 size, u32 at) {
  if (Options::BOUNDS_CHECK && at + size > endpos()) {
    // warnAt("Out of bounds read...", at, size + at);
    assert(!"Out of bounds read");
    abort();
  }
}
void BinaryReader::alignmentCheck(u32 size, u32 at) {
  if (Options::ALIGNMENT_CHECK && at % size) {
    // TODO: Filter warnings in same scope, only print stack once.
    warnAt((std::string("Alignment error: ") + std::to_string(tell()) +
            " is not " + std::to_string(size) + " byte aligned.")
               .c_str(),
           at, at + size, true);
    rsl::debug_break();
  }
}

void BinaryReader::readerBpCheck(u32 size, s32 trans) {
#ifndef NDEBUG
  for (const auto& bp : mBreakPoints) {
    if (tell() + trans >= bp.offset &&
        tell() + trans + size <= bp.offset + bp.size) {
      printf("Reading from %04u (0x%04x) sized %u\n",
             static_cast<u32>(tell() + trans), static_cast<u32>(tell() + trans),
             static_cast<u32>(size));
      warnAt("Breakpoint hit", tell() + trans, tell() + trans + size);
      rsl::debug_break();
    }
  }
#endif
}

struct BinaryReader::DispatchStack {
  struct Entry {
    u32 jump; // Offset in stream where jumped
    u32 jump_sz;

    std::string handlerName; // Name of handler
    u32 handlerStart;        // Start address of handler
  };

  std::array<Entry, 16> mStack;
  u32 mSize = 0;

  void push_entry(u32 j, std::string_view name, u32 start = 0) {
    Entry& cur = mStack[mSize];
    ++mSize;

    cur.jump = j;
    cur.handlerName = name;
    cur.handlerStart = start;
    cur.jump_sz = 1;
  }
};

void BinaryReader::enterRegion(std::string&& name, u32& jump_save,
                               u32& jump_size_save, u32 start, u32 size) {
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
void BinaryReader::exitRegion(u32 jump_save, u32 jump_size_save) {
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

void BinaryReader::warnAt(const char* msg, u32 selectBegin, u32 selectEnd,
                          bool checkStack) {

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
  u32 lineBegin = selectBegin / 16;
  u32 lineEnd = selectEnd / 16 + !!(selectEnd % 16);

  // Write hex lines
  for (u32 i = lineBegin; i < lineEnd; ++i) {
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

  for (u32 i = lineBegin * 16; i < selectBegin; ++i)
    fprintf(stderr, "   ");

  {
    ScopedFormatter fmt(0xa);

    fprintf(stderr, selectEnd - selectBegin == 0
                        ? "^ "
                        : "^~"); // one less, one over below
    for (u32 i = selectBegin + 1; i < selectEnd; ++i)
      fprintf(stderr, "~~~");
  }

  for (u32 i = selectEnd; i < lineEnd * 16; ++i)
    fprintf(stderr, "   ");

  fprintf(stderr, " ");

  for (u32 i = lineBegin * 16; i < selectBegin; ++i)
    fprintf(stderr, " ");

  {
    ScopedFormatter fmt(0xa);

    fprintf(stderr, "^");
    for (u32 i = selectBegin + 1; i < selectEnd; ++i)
      fprintf(stderr, "~");
  }
  fprintf(stderr, "\n");

  // Stack trace

  if (checkStack) {
    beginError();
    describeError("Warning", msg, "");
    addErrorStackTrace(selectBegin, selectEnd - selectBegin, "<root>");

    // printf("\tStack Trace\n\t===========\n");
    if (mStack != nullptr) {
      auto& mStack = *this->mStack;
      for (s32 i = mStack.mSize - 1; i >= 0; --i) {
        const auto& entry = mStack.mStack[i];
        printf("\t\tIn %s: start=0x%X, at=0x%X\n",
               entry.handlerName.size() ? entry.handlerName.c_str() : "?",
               entry.handlerStart, entry.jump);
        if (entry.jump != entry.handlerStart)
          addErrorStackTrace(entry.jump, entry.jump_sz, "indirection");
        addErrorStackTrace(entry.handlerStart, 0,
                           entry.handlerName.size() ? entry.handlerName.c_str()
                                                    : "?");

        if (entry.jump != selectBegin &&
            (i == static_cast<s32>(mStack.mSize) - 1 ||
             mStack.mStack[i + 1].jump != entry.jump))
          warnAt("STACK TRACE", entry.jump, entry.jump + entry.jump_sz, false);
      }
    }

    endError();
  }
}

BinaryReader::BinaryReader(ByteView&& view)
    : ErrorEmitter(*view.getProvider()), mView(std::move(view)) {}
BinaryReader ::~BinaryReader() = default;

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
  F_(u8)                                                                       \
  F_(s8)                                                                       \
  F_(u16)                                                                      \
  F_(s16)                                                                      \
  F_(u32)                                                                      \
  F_(s32)                                                                      \
  F_(f32)

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

template <typename T, EndianSelect E, bool unaligned>
T readImpl(oishii::BinaryReader& reader) {
  T decoded = reader.peek<T, E, unaligned>();
  reader.seek<Whence::Current>(sizeof(T));
  return decoded;
}

#define READ_IMPL_TEU(T_, E_, U_)                                              \
  template <> T_ BinaryReader::read<T_, E_, U_>() {                            \
    return readImpl<T_, E_, U_>(*this);                                        \
  }
#define READ_IMPL_T(T)                                                         \
  READ_IMPL_TEU(T, EndianSelect::Current, false)                               \
  READ_IMPL_TEU(T, EndianSelect::Current, true)                                \
  READ_IMPL_TEU(T, EndianSelect::Big, false)                                   \
  READ_IMPL_TEU(T, EndianSelect::Big, true)                                    \
  READ_IMPL_TEU(T, EndianSelect::Little, false)                                \
  READ_IMPL_TEU(T, EndianSelect::Little, true)

FOR_OISHII_TYPES(READ_IMPL_T)

} // namespace oishii
