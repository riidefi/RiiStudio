#pragma once

#include <vendor/llvm/ADT/DenseMap.h>

namespace rsl {

//! Dense-Map ADT
template<typename K, typename V>
using dense_map = llvm::DenseMap<K, V>;

}