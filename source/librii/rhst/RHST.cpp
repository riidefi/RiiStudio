#include "RHST.hpp"
#include <oishii/reader/binary_reader.hxx>
#include <rsl/TaggedUnion.hpp>

namespace librii::rhst {

enum class RHSTToken {
  RHST_DATA_NULL = 0,

  RHST_DATA_DICT = 1,
  RHST_DATA_ARRAY = 2,
  RHST_DATA_ARRAY_DYNAMIC = 3,

  RHST_DATA_END_DICT = 4,
  RHST_DATA_END_ARRAY = 5,
  RHST_DATA_END_ARRAY_DYNAMIC = 6,

  RHST_DATA_STRING = 7,
  RHST_DATA_S32 = 8,
  RHST_DATA_F32 = 9
};

class RHSTReader {
public:
  RHSTReader(oishii::ByteView file_data) : mReader(std::move(file_data)) {
    mReader.setEndian(std::endian::little);
    mReader.skip(8);
  }
  ~RHSTReader() = default;

  //! A null token. Signifies termination of the file.
  struct NullToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_NULL> {};

  //! A string token. An inline string will directly succeed it.
  struct StringToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_STRING> {
    std::string_view data{};
  };

  //! A 32-bit signed integer token.
  struct S32Token : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_S32> {
    s32 data = -1;
  };

  //! A 32-bit floating-point token.
  struct F32Token : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_F32> {
    f32 data = 0.0f;
  };

  //! Signifies that a dictionary will follow.
  struct DictBeginToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_DICT> {
    std::string_view name;
    u32 size;
  };

  //! Signifies that a dictionary is over. Redundancy
  struct DictEndToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_END_DICT> {};

  //! Signifies that an array will follow.
  struct ArrayBeginToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_ARRAY> {
    u32 size;
  };

  //! Signifies that an array is over. Redundancy
  struct ArrayEndToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_END_ARRAY> {};

  //! Any token	of the many supported.
  struct Token : public rsl::kawaiiUnion<RHSTToken,
                                         // All token types
                                         NullToken, StringToken, S32Token,
                                         F32Token, DictBeginToken, DictEndToken,
                                         ArrayBeginToken, ArrayEndToken> {};

  void readTok() { mLastToken = _readTok(); }
  Token get() const { return mLastToken; }

  template <typename T> const T* expect() {
    readTok();
    return rsl::dyn_cast<T>(mLastToken);
  }

  bool expectSimple(RHSTToken type) {
    readTok();
    return mLastToken.getType() == type;
  }

  bool expectString() { return expectSimple(RHSTToken::RHST_DATA_STRING); }
  bool expectInt() { return expectSimple(RHSTToken::RHST_DATA_S32); }
  bool expectFloat() { return expectSimple(RHSTToken::RHST_DATA_F32); }

private:
  std::string_view _readString() {
    const u32 string_len = mReader.read<u32>();
    mReader.skip(roundUp(string_len, 4));

    const char* string_begin =
        reinterpret_cast<const char*>(mReader.getStreamStart());
    std::string_view data{string_begin, string_len};

    return data;
  }

  Token _readTok() {
    const RHSTToken type = static_cast<RHSTToken>(mReader.read<s32>());

    switch (type) {
    case RHSTToken::RHST_DATA_STRING: {
      std::string_view data = _readString();

      return {StringToken{.data = data}};
    }
    case RHSTToken::RHST_DATA_S32: {
      const s32 data = mReader.read<s32>();

      return {S32Token{.data = data}};
    }
    case RHSTToken::RHST_DATA_F32: {
      const f32 data = mReader.read<f32>();

      return {F32Token{.data = data}};
    }
    case RHSTToken::RHST_DATA_DICT: {
      const u32 nchildren = mReader.read<u32>();
      std::string_view name = _readString();

      return {DictBeginToken{.name = name, .size = nchildren}};
    }
    case RHSTToken::RHST_DATA_END_DICT: {
      return {DictEndToken{}};
    }
    case RHSTToken::RHST_DATA_ARRAY: {
      const u32 nchildren = mReader.read<u32>();
      [[maybe_unused]] const u32 child_type = mReader.read<u32>();

      return {ArrayBeginToken{.size = nchildren}};
    }
    case RHSTToken::RHST_DATA_END_ARRAY: {
      return {ArrayEndToken{}};
    }
    default:
    case RHSTToken::RHST_DATA_NULL: {
      return {NullToken{}};
    }
    }
  }

private:
  oishii::BinaryReader mReader;
  Token mLastToken{NullToken{}};
};

class SceneTreeReader {
public:
  SceneTreeReader(RHSTReader& reader) : mReader(reader) {}

  bool arrayIter(auto functor) {
    auto* array_begin = mReader.expect<RHSTReader::ArrayBeginToken>();
    if (array_begin == nullptr)
      return false;

    for (int i = 0; i < array_begin->size; ++i) {
      if (!functor(i))
        return false;
    }

    auto* array_end = mReader.expect<RHSTReader::ArrayEndToken>();
    if (array_end == nullptr)
      return false;

    return true;
  }

  bool dictIter(auto functor) {
    auto* begin = mReader.expect<RHSTReader::DictBeginToken>();
    if (begin == nullptr)
      return false;

    for (int i = 0; i < begin->size; ++i) {
      if (!functor(begin->name, i))
        return false;
    }

    auto* end = mReader.expect<RHSTReader::DictEndToken>();
    if (end == nullptr)
      return false;

    return true;
  }

  bool read() {
    return dictIter([&](std::string_view domain, int index) {
      if (domain == "head") {
        return dictIter([](std::string_view domain, int index) {
          //
          return false;
        });
      } else if (domain == "body") {
        return false;
      } else {
        return false;
      }
    });
  }

private:
  RHSTReader& mReader;
};

std::optional<SceneTree> ReadSceneTree(std::span<const u8> file_data,
                                       std::string& error_message) {
  std::vector<u8> data_vec(file_data.size());
  memcpy(data_vec.data(), file_data.data(), data_vec.size());
  oishii::DataProvider provider(std::move(data_vec));

  RHSTReader reader(provider.slice());

  SceneTreeReader scn_reader(reader);

  if (!scn_reader.read())
    return std::nullopt;

  return std::nullopt;
}

} // namespace librii::rhst
