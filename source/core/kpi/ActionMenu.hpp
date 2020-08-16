#pragma once

#include <core/kpi/Node2.hpp>

namespace kpi {

// Simple design: only one may exist at once, as an exclusive modal.
//
struct IActionMenu {
  virtual ~IActionMenu() = default;
  virtual bool in_domain(const kpi::IObject& obj) const = 0;

  // Return true if the state of obj was mutated (add to history)
  virtual bool context(kpi::IObject& obj) = 0;
  virtual bool modal(kpi::IObject& obj) = 0;
};

template <typename T> struct ActionMenu : public IActionMenu {
  bool in_domain(const kpi::IObject& obj) const override {
    return dynamic_cast<const T*>(&obj) != nullptr;
  }
};

class ActionMenuManager {
public:
  static ActionMenuManager& get() { return sInstance; }
  
  void addMenu(std::unique_ptr<IActionMenu> menu) {
    menus.emplace_back(std::move(menu));
  }

  // Returns if obj was mutated
  bool drawContextMenus(kpi::IObject& obj) const {
    bool changed = false;
    for (auto& menu : menus)
      if (menu->in_domain(obj))
        changed |= menu->context(obj);
    return changed;
  }

  // Returns if obj was mutated
  bool drawModals(kpi::IObject& obj) const {
    bool changed = false;
    for (auto& menu : menus)
      if (menu->in_domain(obj))
        changed |= menu->modal(obj);
    return changed;
  }

private:
  static ActionMenuManager sInstance;

  std::vector<std::unique_ptr<IActionMenu>> menus;
};

} // namespace kpi
