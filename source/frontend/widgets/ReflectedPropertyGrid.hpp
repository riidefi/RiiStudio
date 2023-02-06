#pragma once

#include <imcxx/Widgets.hpp>
#include <rsl/Annotations.hpp>
#include <rsl/Reflection.hpp>

namespace riistudio::frontend {

struct ReflectedPropertyGrid {
  void HandleTag(auto&& x, auto& name)
    requires std::is_base_of_v<annotations_detail::AnnoTag,
                               std::remove_cvref_t<decltype(x)>>
  {
    if (x.value().starts_with("@")) {
      return;
    }
    name = x.value();
    return;
  }
  void HandleFloatlike(auto&& x, std::string name)
    requires std::is_same_v<std::remove_cvref_t<decltype(x)>, float>
  {
    float data = x;
    ImGui::InputFloat(name.c_str(), &data);
    x = data;
  }
  void HandleEnum(auto&& x, std::string name)
    requires std::is_enum_v<std::remove_cvref_t<decltype(x)>>
  {
    x = imcxx::EnumCombo(name, x);
  }
  void HandleIntegral(auto&& x, std::string name)
    requires std::is_integral_v<std::remove_cvref_t<decltype(x)>>
  {
    int data = x;
    ImGui::InputInt(name.c_str(), &data);
    x = data;
  }

  void Draw(auto&& some_struct) {
    std::string name = "?";
    int i = 0;
    bool last_named = false;
    cista::for_each_field(some_struct, [&](auto&& x) {
      if (last_named) {
        last_named = false;
      } else {
        name = std::format("Field {}", i++);
      }
      if constexpr (std::is_base_of_v<annotations_detail::AnnoTag,
                                      std::remove_cvref_t<decltype(x)>>) {
        HandleTag(x, name);
        last_named = true;
        return;
      }

      if constexpr (std::is_enum_v<std::remove_cvref_t<decltype(x)>>) {
        HandleEnum(x, name);
        return;
      }
      if constexpr (std::is_same_v<std::remove_cvref_t<decltype(x)>, float>) {
        HandleFloatlike(x, name);
        return;
      }
      if constexpr (std::is_integral_v<std::remove_cvref_t<decltype(x)>>) {
        HandleIntegral(x, name);
        return;
      }
    });
  }
};

} // namespace riistudio::frontend
