#pragma once

#include "types.hxx"
#include <assert.h>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

namespace oishii {

class DataProvider;

class ByteView : public std::span<const u8> {
public:
  //! Construct a root-level ByteView.
  ByteView(std::span<const u8> base, DataProvider& provider,
           std::string_view name = "<anonymous view>")
      : std::span<const u8>(base), mParent(nullptr), mProvider(&provider),
        mName(name) {}
  //! Construct a ByteView as a child of an existing instance.
  ByteView(std::span<const u8> base, ByteView& parent,
           std::string_view name = "<anonymous view>")
      : std::span<const u8>(base), mParent(&parent),
        mProvider(parent.mProvider), mName(name) {}

  //! Determine if an offset is in bounds
  bool isInBounds(std::size_t offset) const { return offset < this->size(); }

  const DataProvider* getProvider() const { return mProvider; }

  //! Get the file offset of an element in the view.
  std::size_t providerOffset(std::size_t local_offset = 0) const;

  //! Return the parent, if any.
  const ByteView* getParent() const { return mParent; }

private:
  // Pointer to the parent, for stack traces.
  ByteView* mParent = nullptr;
  DataProvider* mProvider = nullptr;
  //
  std::string_view mName;
}; // namespace oishii

//! Manages the data read from a file.
class DataProvider {
public:
  //! Construct a `DataProvider` from a vector of data.
  DataProvider(std::vector<u8>&& data,
               std::string_view file_path = "<unknown file>")
      : mData(std::move(data)), mPath(file_path) {}

  //! Get a read-only slice of the data.
  ByteView slice(std::size_t start = 0,
                 std::size_t extent = std::dynamic_extent) {
    const std::size_t adjusted_size =
        extent == std::dynamic_extent ? mData.size() : extent;
    std::span<const u8> sliced_span{mData.data() + start, adjusted_size};
    return {sliced_span, *this, mPath};
  }

  std::string_view getFilePath() const { return mPath; }

  // For ByteView to compute file offsets.
  std::ptrdiff_t computeOffset(const u8* element) const {
    assert(element < mData.data() + mData.size() &&
           "element is out of bounds.");
    if (element > mData.data() + mData.size())
      return 0;
    return element - mData.data();
  }

private:
  // We don't keep track of slices, which would hold dangling pointers if mData
  // reallocated.
  const std::vector<u8> mData;

  std::string_view mPath;
};

// Relies on DataProvider definition
inline std::size_t ByteView::providerOffset(std::size_t local_offset) const {
  assert(local_offset < this->size() &&
         "local_offset is out of the bounds of the view.");
  assert(mProvider != nullptr &&
         "Cannot compute provider offset without a provider.");
  if (local_offset >= this->size() || mProvider == nullptr)
    return 0;
  return static_cast<std::size_t>(
      mProvider->computeOffset(this->data() + local_offset));
}

} // namespace oishii
