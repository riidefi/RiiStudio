#pragma once

#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>

namespace riistudio::arc {

enum class EntryType { File, Folder };

struct NormalizedPath : std::string {
  NormalizedPath(const std::filesystem::path& path)
      : std::string(normalizePath(path)) {}

  bool operator==(const NormalizedPath&) const = default;

  // Lexically normalizes a path, with two additional processes:
  // - Removes trailing separators.
  // - Formats the string in lowercase; the FST is case-insensitive.
  //
  static std::string normalizePath(const std::filesystem::path& _path) {
    auto path = _path.lexically_normal().string();
    if (path.ends_with(std::filesystem::path::preferred_separator))
      path.resize(path.size() - 1);
    for (auto& c : path)
      c = tolower(c);
    return path;
  }
};

class LinearFST {
public:
  using Path = std::filesystem::path;

  class FstNode {
  public:
    FstNode(std::unique_ptr<kpi::IMementoOriginator>&& data)
        : type(EntryType::File), file_data(std::move(data)) {}
    FstNode(EntryType etype) : type(etype) {}
    FstNode() : type(EntryType::File) {}

    EntryType getEntryType() const { return type; }
    bool isFolder() const { return type == EntryType::Folder; }
    bool isFile() const { return type == EntryType::File; }

    void addChild(const Path& child) { children.emplace(child); }
    void removeChild(const Path& child) { children.erase(child); }

    const std::set<NormalizedPath>& getChildren() const {
      assert(isFolder());
      return children;
    }

    const kpi::IMementoOriginator* getFileData() const {
      assert(isFile());
      return file_data.get();
    }
    kpi::IMementoOriginator* getFileData() {
      assert(isFile());
      return file_data.get();
    }
    void setFileData(std::unique_ptr<kpi::IMementoOriginator>&& data) {
      file_data = std::move(data);
    }

    //
    // History machinery
    //

    class FstNodeMemento {
      friend class FstNode;

    public:
      FstNodeMemento(const FstNode& in,
                     const kpi::IMemento* last_memento = nullptr)
          : type(in.type), children(in.children) {
        if (in.file_data != nullptr) // folders will be null
          file_data = in.file_data->next(last_memento);
      }
      FstNodeMemento() = default;

    private:
      EntryType type = EntryType::File;
      std::set<NormalizedPath> children;

      std::shared_ptr<const kpi::IMemento> file_data = nullptr;
    };

    FstNodeMemento createMemento(const FstNodeMemento* last = nullptr) const {
      if (last == nullptr)
        return {*this};

      auto last_file_data = last->file_data;
      return {*this, &*last->file_data};
    }

    void setFromMemento(const FstNodeMemento& in) {
      type = in.type;
      children = in.children;
      file_data->from(*in.file_data);
    }

    FstNode(const FstNodeMemento& in) : type(in.type), children(in.children) {
      printf("Oversight in current design: We can't recover from a memnto "
             "originator being deleted\n");
      abort();
    }

  private:
    EntryType type;
    // RawBinaryOriginator if unrecognized
    std::unique_ptr<kpi::IMementoOriginator> file_data;
    // folder data
    std::set<NormalizedPath> children;
  };

  auto fstNodeSentinel() const { return mEntries.end(); }

  auto findFstNode(const Path& path) { return mEntries.find(path); }
  auto findFstNode(const Path& path) const { return mEntries.find(path); }

  FstNode& emplaceFstNode(const Path& path, FstNode&& node) {
    return mEntries[path] = std::move(node);
  }
  FstNode& emplaceFstNode(const Path& path) { return mEntries[path]; }

  //! Erase all references to a node
  void orphanFstNode(const Path& normalized) {
    for (auto& [key, value] : mEntries) {
      value.removeChild(normalized);
    }
  }

  //! Erase a node from the table
  void eraseFstEntry(const Path& path) { mEntries.erase(path); }

  //
  // An archive can have files of a variety of types, perhaps set by plugins!
  // Provides an adapter for non-typed dynamic folder nodes.
  //

  struct FstMemento : public kpi::IMemento {
    std::map<NormalizedPath, FstNode::FstNodeMemento> entries;
  };

  void
  compareEntries(const FstMemento& memento,
                 std::vector<std::string_view>& transient_only_folders,
                 std::vector<std::string_view>& record_only_folders) const {
    for (auto& node : mEntries) {
      if (!memento.entries.contains(node.first))
        transient_only_folders.push_back(node.first);
    }
    for (auto& node : memento.entries) {
      if (!mEntries.contains(node.first))
        record_only_folders.push_back(node.first);
    }

#ifdef BUILD_DEBUG
    printf("Transient-Only Entries:\n");
    for (auto& f : transient_only_folders)
      printf("    %s\n", f.data());
    printf("Record-Only Entries:\n");
    for (auto& f : record_only_folders)
      printf("    %s\n", f.data());
#endif
  }

  void setFromMemento(const FstMemento& memento) {
    std::vector<std::string_view> transient_only_folders, record_only_folders;

    compareEntries(memento, transient_only_folders, record_only_folders);

    // Dangerous: Cannot undo
    assert(transient_only_folders.empty());
    // for (auto& f : transient_only_folders)
    //   mEntries.erase(std::string(f));

    assert(record_only_folders.empty());
    // for (auto& f : record_only_folders)
    //   mEntries.emplace(memento.entries.at(std::string(f)));

    // With the entries synchronized...
    for (auto& [key, value] : mEntries)
      value.setFromMemento(memento.entries.at(key));
  }
  std::unique_ptr<FstMemento>
  createMemento(const FstMemento* last_memento = nullptr) const {
    auto result = std::make_unique<FstMemento>();

    if (last_memento != nullptr) {
      std::vector<std::string_view> transient_only_folders, record_only_folders;

      compareEntries(*last_memento, transient_only_folders,
                     record_only_folders);

      // Dangerous: Cannot undo
      assert(transient_only_folders.empty());
      // for (auto& f : transient_only_folders)
      //   mEntries.erase(std::string(f));

      assert(record_only_folders.empty());
      // for (auto& f : record_only_folders)
      //   mEntries.emplace(memento.entries.at(std::string(f)));
    }

    if (last_memento == nullptr) {
      for (auto& [key, value] : mEntries)
        result->entries[key] = value.createMemento();
    } else {
      for (auto& [key, value] : mEntries) {
        const auto* last = last_memento->entries.contains(key)
                               ? &last_memento->entries.at(key)
                               : nullptr;
        result->entries[key] = value.createMemento(last);
      }
    }

    return result;
  }

private:
  std::map<NormalizedPath, FstNode> mEntries;
};

} // namespace riistudio::arc
