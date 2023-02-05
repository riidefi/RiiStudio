#include <core/common.h>
#include <rsl/Crc32.hpp>

IMPORT_STD;
#include <fstream>
#include <mutex>

namespace riistudio {

struct LocalEntry {
  u32 src_crc32;
  std::string dest_string;

  LocalEntry(u32 hash, std::string d) : src_crc32(hash), dest_string(d) {}

  bool operator<(const LocalEntry& rhs) const {
    return src_crc32 < rhs.src_crc32;
  }
  bool operator<(u32 crc) const { return src_crc32 < crc; }
};

// LS("FROM", "TO") -> LocalEntry
#define LS(a, b)                                                               \
  { rsl::crc32(std::string_view(a, sizeof(a) - 1)), b }

bool gJapaneseLocaleReady = false;
std::vector<LocalEntry> gJapaneseLocale;

static bool replace(std::string& str, const std::string& from,
                    const std::string& to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

// Replace "\0" with '\0'
static void ProcessName(std::string& string) {
  while (replace(string, "\\0", "\1")) {
    // Loop until no more changes
  }

  if (string.size() > 2) {
    // "..." -> ...
    string = string.substr(1, string.size() - 2);
  }

  // Convert \1 to \0
  for (char* c = &string[0]; c != &string[string.size()]; ++c) {
    if (*c == '\1')
      *c = '\0';
  }
}

static void ReadLocale(std::vector<LocalEntry>& dst, const char* path) {
  std::ifstream file(path, std::ios_base::in);

  std::string en;
  std::string jp;

  std::string line;
  while (std::getline(file, line)) {
    auto pos = line.find(",");
    if (pos == std::string::npos)
      return;

    en = line.substr(0, pos);
    jp = line.substr(pos + 1, line.size() - pos - 1);

    ProcessName(en);
    ProcessName(jp);
    gJapaneseLocale.emplace_back(rsl::crc32(en), jp);
  }
}

void ResetJapaneseRemap() {
  gJapaneseLocale.clear();
  gJapaneseLocaleReady = false;
}

static std::optional<std::span<LocalEntry>> GetJapaneseRemap() {
  if (!gJapaneseLocaleReady) {
    ReadLocale(gJapaneseLocale, "lang/jp.csv");
    std::sort(std::begin(gJapaneseLocale), std::end(gJapaneseLocale));
    gJapaneseLocaleReady = true;
  }

  if (gJapaneseLocale.empty())
    return std::nullopt;

  return gJapaneseLocale;
}

class LocaleManager {
public:
  // List of valid locales
  const std::set<std::string>& getLocales() const { return mLocales; }

  // Current locale
  std::string getLocale() const { return mCurLocale; }
  void setLocale(std::string s) {
    if (!mLocales.contains(s)) {
      // Not a valid locale
      rsl::error("Invalid locale string {}", s.c_str());
      return;
    }

    mCurLocale = s;
  }

  // Locale data
  std::optional<std::span<LocalEntry>> getLocaleRemapData() {
    return getLocaleRemapData(mCurLocale);
  }

private:
  std::optional<std::span<LocalEntry>> getLocaleRemapData(std::string_view s) {
    if (s == "English") {
      return std::nullopt;
    }

    if (s == "Japanese") {
      return GetJapaneseRemap();
    }

    // Invalid locale
    return std::nullopt;
  }

private:
  std::string mCurLocale = "English";
  const std::set<std::string> mLocales = {"English", "Japanese"};
};

class LocalizationManager {
public:
  const char* translateString(std::string_view str) {
    auto remap = mLocale.getLocaleRemapData();

    // English
    if (!remap) {
      return str.data();
    }

    // Okay, so we strip a potential trailing space:
    const bool trailing_space = false;
    // str.ends_with(" ");
    auto str_fixed = str.substr(0, str.length() - trailing_space);

    const u32 string_hash = rsl::crc32(str_fixed);
    auto it = std::lower_bound(remap->begin(), remap->end(), string_hash);

    if (it == remap->end() || it->src_crc32 != string_hash) {
      if (!mMissCache.contains(std::string(str)))
        mMissCache[std::string(str)]; // It's a miss
      return str.data();
    }

    // TODO: Linear probe for hash collisions (very unlikely)

    // if (trailing_space) {
    //  auto hacked_str = mTrailingSpaceHacks.find(string_hash);
    //
    //  if (hacked_str != mTrailingSpaceHacks.end()) {
    //    return hacked_str->second.get();
    //  }
    //
    //  auto new_hacked_str = std::make_unique<char[]>(it->dest_string.length()
    //  +
    //                                                 1 /* Trailing space */ +
    //                                                 1 /* Null terminator */);
    //  assert(new_hacked_str);
    //  if (!new_hacked_str)
    //    return "Out of memory";
    //
    //  memcpy(new_hacked_str.get(), it->dest_string.data(),
    //         it->dest_string.length());
    //  new_hacked_str.get()[it->dest_string.length()] = ' ';
    //  new_hacked_str.get()[it->dest_string.length() + 1] = '\0';
    //
    //  mTrailingSpaceHacks.emplace(string_hash, new_hacked_str);
    //
    //  return mTrailingSpaceHacks.find(string_hash)->second.get();
    //}

    return it->dest_string.data();
  }

  void dumpMisses() const {
    std::ofstream file("lang/jp_UNTRANSLATED.csv");
    for (auto& miss : mMissCache) {
      auto str = miss.first;
      for (char* c = &str[0]; c != &str[str.size()]; ++c) {
        if (*c == '\0')
          *c = '\1';
      }
      while (replace(str, "\1", "\\0")) {
        // Repeat until done
      }
      file << "\"" << str << "\",\n";
    }
  }

  LocaleManager& getLocale() { return mLocale; }
  const LocaleManager& getLocale() const { return mLocale; }

private:
  LocaleManager mLocale;
  std::unordered_map<std::string, int> mMissCache;
  std::unordered_map<u32, std::unique_ptr<char[]>> mTrailingSpaceHacks;
};

std::atomic<bool> sLocaleAPIReady;

bool IsLocaleAPIReady() { return sLocaleAPIReady; }
void MarkLocaleAPIReady() { sLocaleAPIReady = true; }

std::mutex gLocalizationMutex;
LocalizationManager gLocalizationManager;

LocalizationManager& GetLocalizationManager() { return gLocalizationManager; }

const char* translateString(std::string_view str) {
  assert(IsLocaleAPIReady());

  std::unique_lock g(gLocalizationMutex);
  return GetLocalizationManager().translateString(str);
}

void SetLocale(std::string s) {
  assert(IsLocaleAPIReady());

  std::unique_lock g(gLocalizationMutex);
  GetLocalizationManager().getLocale().setLocale(s);
}

std::string GetLocale() {
  assert(IsLocaleAPIReady());

  std::unique_lock g(gLocalizationMutex);
  return GetLocalizationManager().getLocale().getLocale();
}

void DebugDumpLocaleMisses() {
  assert(IsLocaleAPIReady());

  std::unique_lock g(gLocalizationMutex);
  return GetLocalizationManager().dumpMisses();
}

void DebugReloadLocales() {
  assert(IsLocaleAPIReady());

  std::unique_lock g(gLocalizationMutex);
  ResetJapaneseRemap();
}

} // namespace riistudio
