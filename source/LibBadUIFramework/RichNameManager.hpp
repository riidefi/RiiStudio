#pragma once

#include "Node2.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <imgui/imgui.h> // ImVec4

namespace kpi {

class RichNameManager {
  static RichNameManager sInstance;
  struct IEntry {
    virtual ~IEntry() = default;
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
    ImVec4 color;

    virtual bool isInDomain(const IObject* node) const = 0;
  };

  template <typename T> struct EntryImpl final : public IEntry {
    // TODO: This solution cannot handle name overrides.
    bool isInDomain(const IObject* node) const override {
      return dynamic_cast<const T*>(node) != nullptr;
    }
    EntryImpl(const std::string_view icon_, const std::string_view name_,
              ImVec4 color_) {
      icon.sg = icon_;
      name.sg = name_;
      color = color_;
    }
  };

public:
  virtual ~RichNameManager() = default;

  static inline RichNameManager& getInstance() { return sInstance; }

  template <typename T>
  inline RichNameManager&
  addRichName(const std::string_view icon, const std::string_view name,
              const std::string_view icon_pl = "",
              const std::string_view name_pl = "",
              ImVec4 icon_color = {1.0f, 1.0f, 1.0f, 1.0f}) {
    auto entry = std::make_unique<EntryImpl<T>>(icon, name, icon_color);
    if (!icon_pl.empty())
      entry->icon.pl = icon_pl;
    if (!name_pl.empty())
      entry->name.pl = name_pl;
    mEntries.push_back(std::move(entry));
    return *this;
  }

  struct EntryDelegate {
    std::string getIconSingular() const {
      return entry != nullptr
                 ? riistudio::translateString(entry->icon.getSingular())
                 : "?";
    }
    std::string getIconPlural() const {
      return entry != nullptr
                 ? riistudio::translateString(entry->icon.getPlural())
                 : "?";
    }

    std::string getNameSingular() const {
      return entry != nullptr
                 ? riistudio::translateString(entry->name.getSingular())
                 : "?";
    }
    std::string getNamePlural() const {
      return entry != nullptr
                 ? riistudio::translateString(entry->name.getPlural())
                 : "?";
    }

    ImVec4 getIconColor() const {
      return entry != nullptr ? entry->color : ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
    }

    bool hasEntry() const { return entry != nullptr; }

  private:
    const IEntry* entry = nullptr;

  public:
    EntryDelegate(const IEntry* _entry) : entry(_entry) {}
  };

  inline EntryDelegate getRich(const IObject* node) const {
    EntryDelegate delegate{getEntry(node)};
    return delegate;
  }

private:
  inline const IEntry* getEntry(const IObject* node) const {
    const auto found =
        std::find_if(mEntries.begin(), mEntries.end(),
                     [&](const auto& x) { return x->isInDomain(node); });
    return found == mEntries.end() ? nullptr : found->get();
  }

  std::vector<std::unique_ptr<IEntry>> mEntries;
};

} // namespace kpi
