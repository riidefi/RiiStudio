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

namespace impl {
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
} // namespace impl

//! Optimized TEV expression simplifier.
//! We may call `solveTevStage()` several times per frame.
class TevExpression {
public:
  TevExpression(const gx::TevStage::ColorStage& substage,
                bool do_print_inter = true) {
    riistudio::util::StringBuilder builder{getStringData(),
                                           sizeof(char) * 1024};
    mExpr = &impl::solveTevStage(substage, builder, mWorkMem.data(),
                                 mWorkMem.size() - sizeof(char) * 1024,
                                 do_print_inter);
    mUsed = impl::computeUsed(*mExpr);
  }
  TevExpression(const gx::TevStage::AlphaStage& substage,
                bool do_print_inter = true) {
    riistudio::util::StringBuilder builder{getStringData(),
                                           sizeof(char) * 1024};
    mExpr = &impl::solveTevStage(substage, builder, mWorkMem.data(),
                                 mWorkMem.size() - sizeof(char) * 1024,
                                 do_print_inter);
    mUsed = impl::computeUsed(*mExpr);
  }

  //! Return a bitmask of RegEx
  u32 computeUsedArguments() const { return mUsed; }
  //! Checks a RegEx mask.
  bool isUsed(u32 arg) const { return mUsed & arg; }
  //! Returns the simplified result (optionally with steps).
  const char* getString() const {
    return reinterpret_cast<const char*>(mWorkMem.data()) +
           impl::TevSolverWorkMemSizeApprox;
  }

private:
  char* getStringData() {
    return reinterpret_cast<char*>(mWorkMem.data()) +
           impl::TevSolverWorkMemSizeApprox;
  }

private:
  Expr* mExpr = nullptr;
  u32 mUsed = 0;
  // last 1024: mString
  std::array<u8, impl::TevSolverWorkMemSizeApprox + sizeof(char) * 1024>
      mWorkMem{};
};

} // namespace libcube::UI
