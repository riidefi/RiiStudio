#pragma once

#include "node.hpp"
#include <string_view>
#include <vector>

namespace kpi {

struct IPropertyView {
  virtual ~IPropertyView() = default;

  virtual bool isInDomain(IDocumentNode *test) const = 0;

  virtual const std::string_view getName() const = 0;
  virtual const std::string_view getIcon() const = 0;
  virtual void draw(kpi::IDocumentNode &active,
                    std::vector<kpi::IDocumentNode *> affected,
                    kpi::History &history, kpi::IDocumentNode &root) = 0;
};

template <typename T> class PropertyDelegate {
public:
  virtual ~PropertyDelegate() = default;

  T &getActive() { return mActive; }
  virtual const T &getActive() const { return mActive; }

  void commit(const char *changeName) {
    ((void)changeName);

    mHistory.commit(mTransientRoot);
  }

  template <typename U, typename TGet, typename TSet>
  void property(const U &before, const U &after, TGet get, TSet set) {
    if (before == after)
      return;

    for (T *it : mAffected) {
      if (!(get(*it) == after)) {
        set(*it, after);
      }
    }

    commit("Property Update");
  }
#define KPI_PROPERTY(delegate, before, after, val)                             \
  delegate.property(                                                           \
      before, after, [&](const auto &x) { return x.##val; },                   \
      [&](auto &x, auto &y) { x.##val = y; })
// When external source updating internal data
#define KPI_PROPERTY_EX(delegate, before, after)                               \
  KPI_PROPERTY(delegate, delegate.getActive().##before, after, before)

private:
  T &mActive;
  std::vector<T *> mAffected;
  kpi::History &mHistory;
  const kpi::IDocumentNode &mTransientRoot;

public:
  PropertyDelegate(T &active, std::vector<T *> affected, kpi::History &history,
                   const kpi::IDocumentNode &transientRoot)
      : mActive(active), mAffected(affected), mHistory(history),
        mTransientRoot(transientRoot) {}
};

template <typename T, typename U>
struct PropertyViewImpl final : public IPropertyView {
  bool isInDomain(IDocumentNode *test) const override {
    return dynamic_cast<T *>(test) != nullptr;
  }
  const std::string_view getName() const override {
    U tmp;
    return tmp.name;
  }
  const std::string_view getIcon() const override {
    U tmp;
    return tmp.icon;
  }
  void draw(kpi::IDocumentNode &active,
            std::vector<kpi::IDocumentNode *> affected, kpi::History &history,
            kpi::IDocumentNode &root) override {
    T *_active = dynamic_cast<T *>(&active);
    assert(_active != nullptr);

    std::vector<T *> _affected(affected.size());
    for (std::size_t i = 0; i < affected.size(); ++i) {
      _affected[i] = dynamic_cast<T *>(affected[i]);
      assert(_affected[i] != nullptr);
    }

    PropertyDelegate<T> delegate(*_active, _affected, history, root);
    U idl_tag;

    drawProperty(delegate, idl_tag);
  }
};
struct PropertyViewManager {
  template <typename TDomain, typename TTag> void addPropertyView() {
    mViews.push_back(std::make_unique<PropertyViewImpl<TDomain, TTag>>());
  }

  template <typename T> void forEachView(T func, kpi::IDocumentNode &active) {
    for (auto &it : mViews) {
      if (!it->isInDomain(&active))
        continue;

      func(*it.get());
    }
  }

  static PropertyViewManager &getInstance() { return sInstance; }

private:
  // In the future we need to adapt for modifier ID
  std::vector<std::unique_ptr<IPropertyView>> mViews;

public:
  static PropertyViewManager sInstance;
};

} // namespace kpi
