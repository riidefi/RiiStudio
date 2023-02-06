#pragma once

#include <core/common.h>
#include <imcxx/Widgets.hpp> // imcxx::Combo

namespace riistudio::frontend {

class PropertyEditorWidget {
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

private:
  void VertTabs() {
    ImGui::BeginChild("Left", ImVec2(120, 0), true);
    {
      imcxx::TabBar bar;
      bar.active = mActiveTab;
      bar.tabs = TabTitles();
      imcxx::DrawVertTabBar(bar);
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

  enum class Mode {
    Tabs,     //!< Standard horizontal tabs
    VertTabs, //!< Tree of tabs on the left
    Headers   //!< Use collapsing headers
  };
  Mode mMode = Mode::VertTabs;

protected:
  int mActiveTab = 0;

private:
  std::vector<bool> tab_filter;
};

} // namespace riistudio::frontend
