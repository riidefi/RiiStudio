#pragma once

#include <array>
#include <memory>
#include <vector>

namespace rsl {

// Assumption: all elements are contiguous--no holes
// Much faster than a vector for the many static sized arrays in materials
template <typename T, size_t N>
struct array_vector_fixed : public std::array<T, N> {
  using base_t = std::array<T, N>;
  size_t size() const { return nElements; }
  void resize(size_t n_elem) {
    assert(n_elem <= N);
    nElements = n_elem;
  }
  bool empty() const { return nElements == 0; }

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

#if 1
  const auto begin() const { return base_t::cbegin(); }
  const auto end() const { return base_t::cbegin() + this->size(); }
  auto begin() { return base_t::begin(); }
  auto end() { return base_t::begin() + this->size(); }
#else
  void begin() const {}
#endif
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

} // namespace rsl
