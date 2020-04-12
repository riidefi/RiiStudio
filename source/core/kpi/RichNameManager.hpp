#pragma once

#include "Node.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace kpi {

class RichNameManager {
  static RichNameManager sInstance;
  struct IEntry {
    template <bool addS> struct Component {
      std::string sg, pl;

      std::string getSingular() const { return !sg.empty() ? sg : "?"; }
      std::string getPlural() const {
        if (pl.empty() && sg.empty())
          return "?";
        return pl.empty() ? (addS ? sg + "s" : sg) : pl;
      }
    };
    Component<false> icon;
    Component<true> name;

    virtual bool isInDomain(const IDocumentNode* node) const = 0;
  };

  template <typename T> struct EntryImpl final : public IEntry {
    // TODO: This solution cannot handle name overrides.
    bool isInDomain(const IDocumentNode* node) const override {
      return dynamic_cast<const T*>(node) != nullptr;
    }
    EntryImpl(const std::string_view icon_, const std::string_view name_) {
      icon.sg = icon_;
      name.sg = name_;
    }
  };

public:
  virtual ~RichNameManager() = default;

  static inline RichNameManager& getInstance() { return sInstance; }

  template <typename T>
  inline RichNameManager& addRichName(const std::string_view icon,
                                      const std::string_view name,
                                      const std::string_view icon_pl = "",
                                      const std::string_view name_pl = "") {
    auto entry = std::make_unique<EntryImpl<T>>(icon, name);
    if (!icon_pl.empty())
      entry->icon.pl = icon_pl;
    if (!name_pl.empty())
      entry->name.pl = name_pl;
    mEntries.push_back(std::move(entry));
    return *this;
  }

  struct EntryDelegate {
    std::string getIconSingular() const {
      return entry != nullptr ? entry->icon.getSingular() : "?";
    }
    std::string getIconPlural() const {
      return entry != nullptr ? entry->icon.getPlural() : "?";
    }

    std::string getNameSingular() const {
      return entry != nullptr ? entry->name.getSingular() : "?";
    }
    std::string getNamePlural() const {
      return entry != nullptr ? entry->name.getPlural() : "?";
    }

  private:
    const IEntry* entry = nullptr;

  public:
    EntryDelegate(const IEntry* _entry) : entry(_entry) {}
  };

  inline EntryDelegate getRich(const IDocumentNode* node) const {
    EntryDelegate delegate{getEntry(node)};
    return delegate;
  }

private:
  inline const IEntry* getEntry(const IDocumentNode* node) const {
    const auto found =
        std::find_if(mEntries.begin(), mEntries.end(),
                     [&](const auto& x) { return x->isInDomain(node); });
    return found == mEntries.end() ? nullptr : found->get();
  }

  std::vector<std::unique_ptr<IEntry>> mEntries;
};

} // namespace kpi
