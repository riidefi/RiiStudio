#pragma once

// !! This should be included first
#include <rsl/Reflection.hpp>

#include <imcxx/Widgets.hpp>
#include <rsl/Annotations.hpp>

namespace riistudio::frontend {

struct ReflectedPropertyGrid {
  enum State {
    NONE,
    NAME,
    KIND,
  };
  State m_state = NONE;
  void HandleTag(auto&& x, auto& name, auto& kind_next)
    requires std::is_base_of_v<annotations_detail::AnnoTag,
                               std::remove_cvref_t<decltype(x)>>
  {
    if (x.value().starts_with("@name")) {
      m_state = NAME;
      return;
    }
    if (x.value().starts_with("@type")) {
      m_state = KIND;
      return;
    }
    if (m_state == NAME) {
      name = x.value();
      m_state = NONE;
      return;
    }
    if (m_state == KIND) {
      kind_next = x.value();
      m_state = NONE;
      return;
    }
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
  void HandleIntegral(auto&& x, std::string name, std::optional<std::string>& type_next)
    requires std::is_integral_v<std::remove_cvref_t<decltype(x)>>
  {
    if (type_next.has_value()) {
      u32 clr = x;

      ImVec4 tmp = ImGui::ColorConvertU32ToFloat4(clr);
      ImGui::ColorEdit4(name.c_str(), &tmp.x);
      clr = ImGui::ColorConvertFloat4ToU32(tmp);

      x = clr;

      type_next = std::nullopt;
      return;
    }
    int data = x;
    ImGui::InputInt(name.c_str(), &data);
    x = data;
  }

  void Draw(auto&& some_struct) {
    std::string name = "?";
    std::optional<std::string> type_next;
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
        HandleTag(x, name, type_next);
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
        HandleIntegral(x, name, type_next);
        return;
      }
    });
  }
};

} // namespace riistudio::frontend
