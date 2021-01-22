#pragma once

#include <imgui/imgui.h>
#include <string>

namespace imcxx {

inline int Combo(const std::string& label, const int current_item,
                 const char* options) {
  int trans_item = current_item;
  ImGui::Combo(label.c_str(), &trans_item, options);
  return trans_item;
}

template <typename T>
inline T Combo(const std::string& label, const T current_item,
               const char* options) {
  return static_cast<T>(Combo(label, static_cast<int>(current_item), options));
}

} // namespace imcxx
