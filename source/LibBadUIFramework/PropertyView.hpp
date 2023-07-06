#pragma once

#include "History.hpp"
#include "Node2.hpp"
#include "Plugins.hpp"
#include <imgui/imgui.h>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace riistudio::frontend {
class EditorWindow;
}

extern bool gIsAdvancedMode;

namespace riistudio::lib3d {
struct Texture;
}

namespace kpi {

template <typename T> class PropertyDelegate {
public:
  virtual ~PropertyDelegate() = default;

  T& getActive() { return mActive; }
  virtual const T& getActive() const { return mActive; }

  template <typename U, typename TGet, typename TSet>
  void property(const U& before, const U& after, TGet get, TSet set) {
    if (before == after)
      return;

    for (T* it : mAffected) {
      if (!(get(*it) == after)) {
        set(*it, after);
      }
    }

    if (ImGui::IsAnyMouseDown()) {
      // Not all property updates come from clicks. But for those that do,
      // postpone a commit until mouse up.
      mPostUpdate();
    } else {
      mCommit("Property Update");
    }
  }
  void commit(const char* s) { mCommit(s); }

  template <typename TGet, typename TSet, typename TVal>
  void propertyAbstract(TGet get, TSet set, const TVal& after) {
    const auto& before = (getActive().*get)();
    const auto prop_get = [get](const auto& x) { return (x.*get)(); };
    const auto prop_set = [set](auto& x, const auto& y) { return (x.*set)(y); };
    property(before, after, prop_get, prop_set);
  }

  template <typename U> static inline U doNothing(U x) { return x; }

  template <typename U, typename V, typename W>
  void propertyEx(U before, U after, V val, W pre = &doNothing) {
    property(
        before, after, [&](const auto& x) { return pre(x).*val; },
        [&](auto& x, auto& y) { (pre(x).*val) = y; });
  }
  // When external source updating internal data
  template <typename U, typename V, typename W>
  void propertyEx(U member, V after, W pre) {
    propertyEx(pre(getActive()).*member, after, member, pre);
  }
  template <typename U, typename V> void propertyEx(U member, V after) {
    propertyEx(getActive().*member, after, member,
               []<typename A>(A& arg) -> A& { return arg; });
  }
#define KPI_PROPERTY(delegate, before, after, val)                             \
  delegate.property(                                                           \
      before, after, [&](const auto& x) { return x.val; },                     \
      [&](auto& x, auto& y) { x.val = y; })
// When external source updating internal data
#define KPI_PROPERTY_EX(delegate, before, after)                               \
  KPI_PROPERTY(delegate, delegate.getActive().before, after, before)

private:
  T& mActive;

public:
  std::vector<T*> mAffected;
  // riistudio::frontend::EditorWindow* mEd;

private:
  std::function<void(void)> mPostUpdate;
  std::function<void(const char*)> mCommit;

public:
  std::function<void(const riistudio::lib3d::Texture*, u32)> mDrawIcon;

  PropertyDelegate(
      std::function<void(void)> postUpdate,
      std::function<void(const char*)> commit, T& active,
      std::vector<T*> affected,
      std::function<void(const riistudio::lib3d::Texture*, u32)> drawIcon)
      : mActive(active), mAffected(affected), mPostUpdate(postUpdate),
        mCommit(commit), mDrawIcon(drawIcon) {}
};

template <typename T>
inline PropertyDelegate<T> MakeDelegate(
    std::function<void(void)> postUpdate,
    std::function<void(const char*)> commit, T* _active,
    std::vector<IObject*> affected,
    std::function<void(const riistudio::lib3d::Texture*, u32)> drawIcon) {
  assert(_active != nullptr);

  std::vector<T*> _affected(affected.size());
  for (std::size_t i = 0; i < affected.size(); ++i) {
    _affected[i] = dynamic_cast<T*>(affected[i]);
    assert(_affected[i] != nullptr);
  }

  PropertyDelegate<T> delegate(postUpdate, commit, *_active, _affected,
                               drawIcon);
  return delegate;
}

} // namespace kpi
