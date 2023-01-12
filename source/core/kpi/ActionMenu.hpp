#pragma once

#include <core/kpi/Node2.hpp>

namespace kpi {

enum ChangeType {
  NO_CHANGE,
  CHANGE,
  CHANGE_NEED_RESET,
};

// Simple design: only one may exist at once, as an exclusive modal.
//
struct IActionMenu {
  virtual ~IActionMenu() = default;
  virtual bool in_domain(const kpi::IObject& obj) const = 0;

  // Return true if the state of obj was mutated (add to history)
  virtual bool context(kpi::IObject& obj) = 0;
  virtual ChangeType modal(kpi::IObject& obj) = 0;

  virtual void set_ed(void* ed) { (void)ed; }
};

template <typename T, typename TDerived>
struct ActionMenu : public IActionMenu {
  bool in_domain(const kpi::IObject& obj) const override {
    return dynamic_cast<const T*>(&obj) != nullptr;
  }
  TDerived& get_self() { return *static_cast<TDerived*>(this); }
  bool context(kpi::IObject& obj) override {
    T* pObj = dynamic_cast<T*>(&obj);
    assert(pObj);
    return get_self().TDerived::_context(*pObj);
  }
  ChangeType modal(kpi::IObject& obj) override {
    T* pObj = dynamic_cast<T*>(&obj);
    assert(pObj);
    const auto res = get_self().TDerived::_modal(*pObj);
    static_assert(std::is_same_v<decltype(res), const bool> ||
                  std::is_same_v<decltype(res), const ChangeType>);
    return static_cast<ChangeType>(res);
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

  // Returns CHANGE if obj was mutated, CHANGE_NEED_RESET if strongly mutated
  ChangeType drawModals(kpi::IObject& obj, void* user) const {
    int changed = NO_CHANGE;
    for (auto& menu : menus) {
      if (menu->in_domain(obj)) {
        menu->set_ed(user);
        changed = std::max(changed, static_cast<int>(menu->modal(obj)));
      }
    }
    return static_cast<ChangeType>(changed);
  }

private:
  static ActionMenuManager sInstance;

  std::vector<std::unique_ptr<IActionMenu>> menus;
};

} // namespace kpi
