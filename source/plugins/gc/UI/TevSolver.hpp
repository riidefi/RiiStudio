#pragma once

#include <core/common.h>                  // for u32
#include <core/util/string_builder.hpp>   // for StringBuilder
#include <plugins/gc/Export/Material.hpp> // gx::TevStage
#include <tuple>                          // for std::pair

namespace libcube::UI {

struct Expr;

enum RegEx {
  // Bitmask, if the following registers are used in an expression.
  A = 1 << 0,
  B = 1 << 1,
  C = 1 << 2,
  D = 1 << 3,

  // Internal
  One = 1 << 4,
  Zero = 1 << 5,
};

// Simplify an expression
bool optimizeNode(Expr& e);
void printExprPoly(Expr& e, riistudio::util::StringBuilder& builder,
                   bool root = false);
u32 computeUsed(const Expr& e);

extern const u32 TevSolverWorkMemSize; // 448, 64-bit
constexpr u32 TevSolverWorkMemSizeApprox = 512;

Expr& solveTevStage(const gx::TevStage::ColorStage& substage,
                    riistudio::util::StringBuilder& builder, u8* workmem,
                    std::size_t workmem_size, bool do_print_inter = true);
Expr& solveTevStage(const gx::TevStage::AlphaStage& substage,
                    riistudio::util::StringBuilder& builder, u8* workmem,
                    std::size_t workmem_size, bool do_print_inter = true);

} // namespace libcube::UI
