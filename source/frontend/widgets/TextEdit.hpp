#pragma once

#include <imcxx/Widgets.hpp>
#include <string>
#include <vector>

namespace riistudio::frontend {

struct TextEdit {
  std::vector<char> m_buffer;

  void Reset(const std::string& name) {
    m_buffer.resize(name.size() + 1024);
    std::fill(m_buffer.begin(), m_buffer.end(), '\0');
    m_buffer.insert(m_buffer.begin(), name.begin(), name.end());
  }

  bool Modal(const std::string& name) {
    auto new_name = std::string(m_buffer.data());
    bool ok = ImGui::InputText(
        name != new_name ? "Name*###Name" : "Name###Name", m_buffer.data(),
        m_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
    return ok;
  }

  std::string String() const { return std::string(m_buffer.data()); }
};

} // namespace riistudio::frontend
