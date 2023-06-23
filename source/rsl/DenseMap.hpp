#pragma once

#if 0
#include <vendor/llvm/ADT/DenseMap.h>
#else
#include <map>
#endif

namespace rsl {

#if 0
//! Dense-Map ADT
template<typename K, typename V>
using dense_map = llvm::DenseMap<K, V>;
#else
template <typename K, typename V>
using dense_map = std::map<K, V>;
#endif

} // namespace rsl
