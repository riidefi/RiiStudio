#pragma once

#define RII_INTERFACE(T)                                                       \
public:                                                                        \
  virtual ~T() = default;                                                      \
                                                                               \
protected:                                                                     \
  T() = default;                                                               \
  T(const T&) = default;                                                       \
  T& operator=(const T&) = default;                                            \
  T(T&&) noexcept = default;                                                   \
  T& operator=(T&&) noexcept = default;                                        \
                                                                               \
private:
