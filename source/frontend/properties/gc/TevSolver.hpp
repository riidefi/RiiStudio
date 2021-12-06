#pragma once

#include <core/common.h>                  // for u32
#include <plugins/gc/Export/Material.hpp> // gx::TevStage
#include <rsl/StringBuilder.hpp>          // for StringBuilder
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
  Half = 1 << 6,
  Two = 1 << 7,
  Four = 1 << 8,
};

namespace impl {
// Simplify an expression
bool optimizeNode(Expr& e);
void printExprPoly(Expr& e, rsl::StringBuilder& builder, bool root = false);
u32 computeUsed(const Expr& e);

extern const u32 TevSolverWorkMemSize; // 448*8, 64-bit
constexpr u32 TevSolverWorkMemSizeApprox = 512 * 8;

Expr& solveTevStage(const gx::TevStage::ColorStage& substage,
                    rsl::StringBuilder& builder, u8* workmem,
                    std::size_t workmem_size, bool do_print_inter = true);
Expr& solveTevStage(const gx::TevStage::AlphaStage& substage,
                    rsl::StringBuilder& builder, u8* workmem,
                    std::size_t workmem_size, bool do_print_inter = true);
} // namespace impl

//! Optimized TEV expression simplifier.
//! We may call `solveTevStage()` several times per frame.
class TevExpression {
public:
  TevExpression(const gx::TevStage::ColorStage& substage,
                bool do_print_inter = true) {
    rsl::StringBuilder builder{getStringData(), sizeof(char) * 1024};
    mExpr = &impl::solveTevStage(substage, builder, mWorkMem.data(),
                                 mWorkMem.size() - sizeof(char) * 1024,
                                 do_print_inter);
    mUsed = impl::computeUsed(*mExpr);
  }
  TevExpression(const gx::TevStage::AlphaStage& substage,
                bool do_print_inter = true) {
    rsl::StringBuilder builder{getStringData(), sizeof(char) * 1024};
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
