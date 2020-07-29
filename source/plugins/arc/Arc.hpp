#pragma once

#include <span>
#include <string>
#include <vector>

#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <plugins/arc/LinearFST.hpp>

namespace riistudio::arc {

//! Adapters to allow unknown file to have a history state
//! Note: Intended for usage with an archive -- single-ownership.
class RawBinaryOriginator : public kpi::IMementoOriginator {
public:
  RawBinaryOriginator(std::span<const u8> src) : mTransient(src) {}
  void setData(std::span<const u8> src) {
    ++mTransient.generation;
    mTransient.data.resize(src.size());
    std::copy(src.begin(), src.end(), mTransient.data.begin());
  }
  std::span<const u8> getData() const { return mTransient.data; }

private:
  struct Data {
    u32 generation = 0;
    std::vector<u8> data;

    Data() = default;
    Data(std::span<const u8> src) : data(src.begin(), src.end()) {}
  };
  struct Memento : public kpi::IMemento {
    std::shared_ptr<const Data> data = nullptr;
    Memento() = default;
    Memento(const Data& in) : data(std::make_shared<const Data>(in)) {}
    Memento(std::shared_ptr<const Data> in) : data(in) {}
  };

  const Memento* asMemento(const kpi::IMemento& memento) const {
    return dynamic_cast<const Memento*>(&memento);
  }

  std::unique_ptr<kpi::IMemento>
  next(const kpi::IMemento* last) const override {
    if (last != nullptr) {
      const Memento* pMemento = asMemento(*last);
      if (pMemento != nullptr &&
          pMemento->data->generation == mTransient.generation) {
        return std::make_unique<Memento>(pMemento->data);
      }
    }

    return std::make_unique<Memento>(mTransient);
  }
  void from(const kpi::IMemento& memento) override {
    const Memento* pMemento = asMemento(memento);
    if (pMemento != nullptr &&
        pMemento->data->generation != mTransient.generation) {
      mTransient = *pMemento->data;
    }
  }

  Data mTransient;
};

//! Naive archive implementation, designed for tracks and other simple archives.
//! Files and folders are stored linearly; this implementation would not be
//! suitable for a disc image archive.
//!
//! Relative paths are entirely supported.
//!
class Archive : protected LinearFST, public kpi::IMementoOriginator {
public:
  using Path = LinearFST::Path;

  //! Return if the path exists.
  bool exists(const Path& path) const;

  //! Return if the path is a folder.
  //!
  //! @pre The path exists in the archive.
  bool isFolder(const Path& path) const;

  //! Return if the path is a file.
  //!
  //! @pre The path exists in the archive.
  bool isFile(const Path& path) const;

  //! Get a valid pointer to the specified file if it exists, otherwise nullptr.
  kpi::IMementoOriginator* getFile(const Path& path);

  //! Get a valid pointer to the specified file if it exists, otherwise nullptr.
  const kpi::IMementoOriginator* getFile(const Path& path) const;

  //! Set a file to the specified data.
  //!
  //! @pre
  //!  - An entry exists with the specified path.
  //!  - The entry is a file.
  //!
  void setFile(const Path& path,
               std::unique_ptr<kpi::IMementoOriginator>&& data);

  //! @brief Delete a file or folder in the archive.
  //!
  //! When deleting a folder, all children will be deleted too.
  //! If the path does not exist, this function returns immediately.
  //!
  void deleteEntry(const Path& path);

  //! @brief Create a folder at the specified path.
  //!
  //! Any necessary parent folders will also be created.
  //!
  //! @return If the operation succeeded. The operation will fail if a file
  //! already exists at a path where a folder must be.
  //!
  bool createFolder(const Path& path);

  //! @brief Create a file at the specified path.
  //!
  //! Folders will be created if necessary
  //!
  //! @return If the operation succeeded. The operation will fail if a folder
  //! already exists at the path.
  //!
  bool createFile(const Path& path,
                  std::unique_ptr<kpi::IMementoOriginator> data = nullptr);

  //! Get the contents of a folder.
  //!
  //! @pre A folder exists at the specified path.
  const std::set<NormalizedPath>& getFolderChildren(const Path& path) const;

private:
  // An archive can have files of a variety of types, perhaps set by plugins!
  // Provides an adapter for non-typed dynamic folder nodes.
  const FstMemento* asMemento(const kpi::IMemento& memento) const {
    return dynamic_cast<const FstMemento*>(&memento);
  }

  std::unique_ptr<kpi::IMemento>
  next(const kpi::IMemento* last) const override {
    if (last != nullptr) {
      const FstMemento* pMemento = asMemento(*last);
      if (pMemento != nullptr) {
        return createMemento(pMemento);
      }
    }

    return createMemento(nullptr);
  }
  void from(const kpi::IMemento& memento) override {
    const FstMemento* pMemento = asMemento(memento);
    assert(pMemento != nullptr);
    if (pMemento != nullptr) {
      setFromMemento(*pMemento);
    }
  }

  const FstNode& getEntry(const Path& path) const;
  FstNode& getEntry(const Path& path);
};

} // namespace riistudio::arc
