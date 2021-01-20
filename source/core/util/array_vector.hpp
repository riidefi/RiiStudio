#pragma once

#include <array>
#include <memory>
#include <vector>

namespace riistudio::util {

// Assumption: all elements are contiguous--no holes
// Much faster than a vector for the many static sized arrays in materials
template <typename T, size_t N>
struct array_vector_fixed : public std::array<T, N> {
  size_t size() const { return nElements; }
  void resize(size_t n_elem) { nElements = n_elem; }

  size_t fixed_size() const { return N; }

  void push_back(T elem) {
    std::array<T, N>::at(nElements) = std::move(elem);
    ++nElements;
  }
  void pop_back() { --nElements; }
  bool operator==(const array_vector_fixed& rhs) const noexcept {
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

protected:
  size_t nElements = 0;
};

template <typename T, size_t N>
struct array_vector_dynamic : public std::vector<T> {
  size_t fixed_size() const { return N; }

  void erase(std::size_t index) {
    assert(index >= 0 && index < this->size());
    for (std::size_t i = index; i < this->size() - 1; ++i) {
      this->at(i) = std::move(this->at(i + 1));
    }
    this->resize(this->size() - 1);
  }
};

#ifndef BUILD_DEBUG
template <typename T, size_t N> using array_vector = array_vector_dynamic<T, N>;
#else
template <typename T, size_t N> using array_vector = array_vector_fixed<T, N>;
#endif

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
    this->resize(rhs.size());
    for (std::size_t i = 0; i < this->size(); ++i) {
      this->at(i) = nullptr;
    }
    for (std::size_t i = 0; i < rhs.size(); ++i) {
      this->at(i) = rhs.at(i)->clone();
    }
    return *this;
  }
  bool operator==(const copyable_polymorphic_array_vector& rhs) const noexcept {
    if (rhs.size() != this->size())
      return false;
    for (int i = 0; i < this->size(); ++i) {
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
