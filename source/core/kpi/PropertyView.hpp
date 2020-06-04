#pragma once

#include "Node.hpp"
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace kpi {

class PropertyViewStateHolder;

struct IPropertyViewState {
  virtual ~IPropertyViewState() = default;
};

struct IPropertyView {
  virtual ~IPropertyView() = default;

  virtual bool isInDomain(IDocumentNode* test) const = 0;

  virtual const std::string_view getName() const = 0;
  virtual const std::string_view getIcon() const = 0;
  virtual void draw(kpi::IDocumentNode& active,
                    std::vector<kpi::IDocumentNode*> affected,
                    kpi::History& history, kpi::IDocumentNode& root,
                    PropertyViewStateHolder& state_holder) = 0;

  virtual std::unique_ptr<IPropertyViewState> constructState() const = 0;
};

template <typename T> class PropertyDelegate {
public:
  virtual ~PropertyDelegate() = default;

  T& getActive() { return mActive; }
  virtual const T& getActive() const { return mActive; }

  void commit(const char* changeName) {
    ((void)changeName);

    mHistory.commit(mTransientRoot);
  }

  template <typename U, typename TGet, typename TSet>
  void property(const U& before, const U& after, TGet get, TSet set) {
    if (before == after)
      return;

    for (T* it : mAffected) {
      if (!(get(*it) == after)) {
        set(*it, after);
      }
    }

    commit("Property Update");
  }

  template <typename T> static inline T doNothing(T x) { return x; }

  template <typename U, typename V, typename W>
  void propertyEx(U before, U after, V val, W pre = &doNothing) {
    property(
        before, after, [&](const auto& x) { return pre(x).*val; },
        [&](auto& x, auto& y) { (pre(x).*val) = y; });
  }
  // When external source updating internal data
  template <typename U, typename V, typename W>
  void propertyEx(U member, V after, W pre = &doNothing) {
    propertyEx(pre(getActive()).*member, after, member, pre);
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

private:
  kpi::History& mHistory;
  const kpi::IDocumentNode& mTransientRoot;

public:
  PropertyDelegate(T& active, std::vector<T*> affected, kpi::History& history,
                   const kpi::IDocumentNode& transientRoot)
      : mActive(active), mAffected(affected), mHistory(history),
        mTransientRoot(transientRoot) {}
};

class PropertyViewStateHolder {
public:
  static constexpr int lifetime_grace_period = 30; // Duration of 0.5 seconds

  void garbageCollect() {
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto& [key, value] : states) {
        if (auto& last_used = value.second;
            last_used++ > lifetime_grace_period) {
          DebugReport("[PropertyViewStateHolder] Destroying state for: %s.\n",
                      std::string(key.id).c_str());
          states.erase(key);
          changed = true;
          break;
        }
      }
    }
  }

  IPropertyViewState* requestState(kpi::IDocumentNode& node,
                                   const IPropertyView& blueprint) {
    Key key{&node, blueprint.getName()};
    if (auto it = states.find(key); it != states.end()) {
      auto& [state, last_used] = it->second;
      last_used = 0;
      return state.get();
    }

    auto state = blueprint.constructState();
    if (state.get() == nullptr) {
      return nullptr;
    }

    DebugReport("[PropertyViewStateHolder] Creating state for: %s.\n",
                std::string(blueprint.getName()).c_str());

    auto* state_ref = state.get();
    states.emplace(key, std::make_pair(std::move(state), 0));
    return state_ref;
  }

private:
  struct Key {
    kpi::IDocumentNode* node;
    std::string_view id;

    bool operator==(const Key& rhs) const {
      return node == rhs.node && id == rhs.id;
    }
  };
  struct KeyHash {
    std::size_t operator()(Key const& key) const noexcept {
      std::size_t h1 = std::hash<kpi::IDocumentNode*>{}(key.node);
      std::size_t h2 = std::hash<std::string_view>{}(key.id);
      return h1 ^ (h2 << 1);
    }
  };
  std::unordered_map<
      Key,
      std::pair<std::unique_ptr<IPropertyViewState>, int>, // <state, last_used>
      KeyHash>
      states;
};

template <typename T> class is_stateful {
  using true_t = u8;
  using false_t = u16;

  template <typename U> static true_t& test(decltype(&U::tag_stateful));
  template <typename U> static false_t& test(...);

public:
  enum { value = sizeof(test<T>(0)) == sizeof(true_t) ? 1 : 0 };
};

template <typename T> constexpr bool is_stateful_v = is_stateful<T>::value != 0;

struct StatefulTestA {
  int tag_stateful;
};
static_assert(is_stateful_v<StatefulTestA>, "Detecting tag_stateful");
struct StatefulTestB {
  int no_tag;
};
static_assert(!is_stateful_v<StatefulTestB>, "Detecting tag_stateful");

template <typename T, typename U>
struct PropertyViewImpl final : public IPropertyView {
  struct ViewStateImpl : public IPropertyViewState {
    U value;
  };

  bool isInDomain(IDocumentNode* test) const override {
    return dynamic_cast<T*>(test) != nullptr;
  }
  const std::string_view getName() const override {
    U tmp;
    return tmp.name;
  }
  const std::string_view getIcon() const override {
    U tmp;
    return tmp.icon;
  }
  void draw(kpi::IDocumentNode& active,
            std::vector<kpi::IDocumentNode*> affected, kpi::History& history,
            kpi::IDocumentNode& root,
            PropertyViewStateHolder& state_holder) override {
    T* _active = dynamic_cast<T*>(&active);
    assert(_active != nullptr);

    std::vector<T*> _affected(affected.size());
    for (std::size_t i = 0; i < affected.size(); ++i) {
      _affected[i] = dynamic_cast<T*>(affected[i]);
      assert(_affected[i] != nullptr);
    }

    PropertyDelegate<T> delegate(*_active, _affected, history, root);
    if constexpr (is_stateful_v<U>) {
      IPropertyViewState* state = state_holder.requestState(active, *this);
      assert(state != nullptr);
      drawProperty(delegate, reinterpret_cast<ViewStateImpl*>(state)->value);
    } else {
      U idl_tag;
      drawProperty(delegate, idl_tag);
    }
  }
  std::unique_ptr<IPropertyViewState> constructState() const override {
    return std::make_unique<ViewStateImpl>();
  }
};

struct PropertyViewManager {
  template <typename TDomain, typename TTag> void addPropertyView() {
    mViews.push_back(std::make_unique<PropertyViewImpl<TDomain, TTag>>());

    // TODO: Sort tabs based on hierarchy
    std::stable_sort(mViews.begin(), mViews.end(),
                     [](const auto& left, const auto& right) {
                       return left->getName()[0] < right->getName()[0];
                     });
  }

  template <typename T> void forEachView(T func, kpi::IDocumentNode& active) {
    for (auto& it : mViews) {
      if (!it->isInDomain(&active))
        continue;

      func(*it.get());
    }
  }

  static PropertyViewManager& getInstance() { return sInstance; }

private:
  // In the future we need to adapt for modifier ID
  std::vector<std::unique_ptr<IPropertyView>> mViews;

public:
  static PropertyViewManager sInstance;
};

} // namespace kpi
