#pragma once

#if 0
#include <type_traits>
#endif

namespace rsl {

// XXX: Do not use this type outside of a union
template <typename TUnion> struct kawaiiUnionBase {
  const TUnion& getType() const { return mShape; }
  TUnion& getType() { return mShape; }
  void setType(TUnion t) { mShape = t; }

private:
  TUnion mShape;
};

template <typename TUnion, typename... T>
struct kawaiiUnion : public kawaiiUnionBase<TUnion>, T... {
  template <typename U> kawaiiUnion(const U& that) {
    static_cast<U&>(*this) = that;
    this->setType(that.gettag());
  }

  // Prevents a copy
  template <typename U
#if 0
			, typename std::is_base_of<kawaiiUnionBase<TUnion>,
			U>::type* sfinae = nullptr
#endif
            >
  kawaiiUnion& operator=(const U& that) {
    static_cast<U&>(*this) = that;
    this->setType(that.gettag());

    return *this;
  }
};

template <typename T, T tag> struct kawaiiTag {
  static bool classof(const kawaiiUnionBase<T>* a) {
    return a->getType() == tag;
  }
  static T gettag() { return tag; }
};

template <typename T, typename U> T* dyn_cast(U& base) {
  return T::classof(&base) ? static_cast<T*>(&base) : nullptr;
}

template <typename T, typename U> bool is_a(U& base) {
  return T::classof(&base);
}

} // namespace rsl
