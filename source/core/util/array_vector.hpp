#pragma once

#include <array>
#include <memory>

namespace riistudio::util {

// Assumption: all elements are contiguous--no holes
// Much faster than a vector for the many static sized arrays in materials
template <typename T, size_t N> struct array_vector : public std::array<T, N> {
  size_t size() const { return nElements; }
  size_t nElements = 0;

  void push_back(T elem) {
    std::array<T, N>::at(nElements) = std::move(elem);
    ++nElements;
  }
  void pop_back() { --nElements; }
  bool operator==(const array_vector& rhs) const noexcept {
    if (rhs.nElements != nElements)
      return false;
    for (int i = 0; i < nElements; ++i) {
      const T& l = this->at(i);
      const T& r = rhs.at(i);
      if (!(l == r))
        return false;
    }
    return true;
  }
  void erase(std::size_t index) {
    assert(index >= 0 && index < nElements);
    for (std::size_t i = index; i < nElements - 1; ++i) {
      std::array<T, N>::at(i) = std::move(std::array<T, N>::at(i + 1));
    }
    --nElements;
  }
};

template <typename T, std::size_t size>
struct copyable_polymorphic_array_vector
    : public array_vector<std::unique_ptr<T>, size> {
#if defined(_MSC_VER) && !defined(__clang__)
  using super = array_vector;
#else
  // MSVC bug?
  using super = array_vector<std::unique_ptr<T>, size>;
#endif
  copyable_polymorphic_array_vector&
  operator=(const copyable_polymorphic_array_vector& rhs) {
    super::nElements = rhs.super::nElements;
    for (std::size_t i = 0; i < super::nElements; ++i) {
      super::at(i) = nullptr;
    }
    for (std::size_t i = 0; i < rhs.super::nElements; ++i) {
      super::at(i) = rhs.super::at(i)->clone();
    }
    return *this;
  }
  bool operator==(const copyable_polymorphic_array_vector& rhs) const noexcept {
    if (rhs.nElements != super::nElements)
      return false;
    for (int i = 0; i < super::nElements; ++i) {
      const T& l = *this->at(i).get();
      const T& r = *rhs.at(i).get();
      if (!(l == r))
        return false;
    }
    return true;
  }
  copyable_polymorphic_array_vector(
      const copyable_polymorphic_array_vector& rhs) {
    *this = rhs;
  }
  copyable_polymorphic_array_vector() = default;
};

} // namespace riistudio::util
