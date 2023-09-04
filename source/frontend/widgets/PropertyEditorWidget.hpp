#pragma once

#include <core/common.h>
#include <imcxx/Widgets.hpp> // imcxx::Combo

namespace riistudio::frontend {

struct PropertyEditorState {
  enum class Mode {
    Tabs,     //!< Standard horizontal tabs
    VertTabs, //!< Tree of tabs on the left
    Headers   //!< Use collapsing headers
  };
  Mode mMode = Mode::VertTabs;
  int mActiveTab = 0;
  std::vector<bool> tab_filter;
};

class PropertyEditorWidget : public PropertyEditorState {
public:
  void DrawTabWidget(bool compact) {
    if (compact)
      ImGui::PushItemWidth(150);

    mMode = imcxx::Combo(compact ? "##Property Mode" : "Property Mode"_j, mMode,
                         "Tabs\0"
                         "Vertical Tabs\0"
                         "Headers\0"_j);
    if (compact)
      ImGui::PopItemWidth();
  }

  virtual std::vector<std::string> TabTitles() = 0;
  virtual bool Tab(int index) = 0;

  void Tabs() {
    if (mMode == Mode::Tabs) {
      HorizTabs();
    } else if (mMode == Mode::VertTabs) {
      VertTabs();
    } else if (mMode == Mode::Headers) {
      HeaderTabs();
    } else {
      ImGui::Text("Unknown mode");
    }
  }

  std::function<void(const char*, int)> m_drawText = [](const char* s,
                                                        int idx) {
    (void)idx;
    ImGui::TextUnformatted(s);
  };

private:
  void VertTabs() {
    ImGui::BeginChild("Left", ImVec2(120, 0), true);
    {
      imcxx::TabBar bar;
      bar.active = mActiveTab;
      bar.tabs = TabTitles();
      for (size_t i = 0; i < bar.tabs.size(); ++i) {
        const bool sel = i == bar.active;
        std::string i_s = "##" + std::to_string(i);
        auto x = ImGui::GetCursorPosX();
        bool s = ImGui::Selectable(i_s.c_str(), sel);
        ImGui::SameLine();
        ImGui::SetCursorPosX(x);
        m_drawText(bar.tabs[i].c_str(), i);
        if (s) {
          bar.active = i;
        }
      }
      mActiveTab = bar.active;
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Right", ImGui::GetContentRegionAvail(), true);
    {
      // Just the current tab
      if (!Tab(mActiveTab)) {
        mActiveTab = 0;
      }
    }
    ImGui::EndChild();
  }
  void HorizTabs() {
    // Active as an input is not strictly needed for horiztabs because imgui
    // caches that
    imcxx::TabBar bar;
    bar.active = mActiveTab;
    bar.tabs = TabTitles();
    imcxx::DrawHorizTabBar(bar);
    mActiveTab = bar.active;
    if (!Tab(mActiveTab)) {
      mActiveTab = 0;
    }
  }
  void HeaderTabs() {
    imcxx::CheckBoxTabBar bar;
    bar.active = tab_filter;
    bar.tabs = TabTitles();

    imcxx::DrawCheckBoxTabBar(bar);
    tab_filter = bar.active;

    std::string title;

    for (int i = 0; i < bar.tabs.size(); ++i) {
      if (!tab_filter[i]) {
        return;
      }

      title = bar.tabs[i];
      bool tmp = tab_filter[i];
      if (ImGui::CollapsingHeader((title + "##_TAB").c_str(), &tmp,
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!Tab(i)) {
          // TODO: Do we want this
          mActiveTab = 0;
        }
      }
      tab_filter[i] = tmp;
    }
  }
};

class Pv2Impl : public PropertyEditorWidget {
public:
  void DrawHeader(bool compact = false) { DrawTabWidget(compact); }
  void DrawBody(std::function<bool(int)> drawTab,
                std::vector<std::string> tabTitles) {
    m_drawTabs = drawTab;
    m_tabTitles = tabTitles;
    Tabs();
  }

private:
  std::vector<std::string> TabTitles() override { return m_tabTitles; }
  bool Tab(int index) override { return m_drawTabs(index); }

  std::function<bool(int)> m_drawTabs = nullptr;
  std::vector<std::string> m_tabTitles;
};
static inline void DrawPropertyEditorWidgetV2_Header(PropertyEditorState& state,
                                                     bool compact = false) {
  Pv2Impl impl;
  static_cast<PropertyEditorState&>(impl) = state;
  impl.DrawHeader(compact);
  state = impl;
}
static inline void
DrawPropertyEditorWidgetV2_Body(PropertyEditorState& state,
                                std::function<bool(int)> drawTab,
                                std::vector<std::string> tabTitles) {
  Pv2Impl impl;
  static_cast<PropertyEditorState&>(impl) = state;
  impl.DrawBody(drawTab, tabTitles);
  state = impl;
}
static inline void
DrawPropertyEditorWidgetV2(PropertyEditorState& state,
                           std::function<bool(int)> drawTab,
                           std::vector<std::string> tabTitles) {
  DrawPropertyEditorWidgetV2_Header(state, false);
  DrawPropertyEditorWidgetV2_Body(state, drawTab, tabTitles);
}

/////////////
// V3 API
/////////////
static inline void DrawPropertyEditorWidgetV3_Body(
    PropertyEditorState& state, std::function<bool(int)> drawTab,
    std::function<void(int)> drawTabTitle, std::vector<std::string> tabTitles) {
  Pv2Impl impl;
  static_cast<PropertyEditorState&>(impl) = state;
  impl.m_drawText = [&](const char* s, int idx) {
    (void)s;
    drawTabTitle(idx);
  };
  impl.DrawBody(drawTab, tabTitles);
  state = impl;
}

} // namespace riistudio::frontend
