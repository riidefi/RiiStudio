#pragma once

#include <string>

namespace rsl {

template <typename T, typename S>
inline void LanguageFeatures(T functor_yes, S functor_no) {
#ifdef rsl_feature_yes
#undef rsl_feature_yes
#endif
#ifdef rsl_feature_no
#undef rsl_feature_no
#endif
#define rsl_feature_yes(name, status) functor_yes(name, status)
#define rsl_feature_no(name) functor_no(name)
#include "versions_lang.hpp"
#undef rsl_feature_yes
#undef rsl_feature_no
}

template <typename T, typename S>
inline void LibraryFeatures(T functor_yes, S functor_no) {
#ifdef rsl_feature_yes
#undef rsl_feature_yes
#endif
#ifdef rsl_feature_no
#undef rsl_feature_no
#endif
#define rsl_feature_yes(name, status) functor_yes(name, status)
#define rsl_feature_no(name) functor_no(name)
#include "versions_lib.hpp"
#undef rsl_feature_yes
#undef rsl_feature_no
}

inline std::string VersionReport() {
  std::string data = "";

  const auto on_yes = [&](const char* name, int status) {
    data += "(X) ";
    data += name;
    data += " = ";
    data += std::to_string(status);
    data += "\n";
  };
  const auto on_no = [&](const char* name) {
    data += "( ) ";
    data += name;
    data += "\n";
  };

  data += "Language Features\n---\n";
  LanguageFeatures(on_yes, on_no);

  data += "Library Features\n---\n";
  LibraryFeatures(on_yes, on_no);

  return data;
}

} // namespace rsl
