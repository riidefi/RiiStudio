#include "TevSolver.hpp"

import std.core;

namespace libcube::UI {

enum class ExprOp : u32 {
  Add,    // L + R
  Sub,    // L - R
  Mul,    // L * R
  Select, // L ? R.L : R.R where R is a Pair
  Pair,   // (L, R)
  Gt,     // (L > R) ? 1 : 0
  Eq,     // (L == R) ? 1 :0
};
struct Expr {
  enum Type : u32 { Unary, Binary };
  Type type;
  Expr* parent = nullptr;
};

namespace impl {

struct UnaryExpr : Expr {
  u32 val = 0;
  // We will never point to another..
  // Expr* ptr = nullptr;

  UnaryExpr(Expr* _parent) {
    parent = _parent;
    type = Type::Unary;
  }
  UnaryExpr() { type = Type::Unary; }
};

struct BinExpr : Expr {
  ExprOp op;
  Expr* left = nullptr;
  Expr* right = nullptr;

  BinExpr(Expr* _parent) {
    parent = _parent;
    type = Type::Binary;
  }
  BinExpr() { type = Type::Binary; }
};

struct ExprAlloc {
  union AnyExpr {
    UnaryExpr unary;
    BinExpr bin;
  };

  // Maximum complexity of a tev stage
  std::array<AnyExpr, 11 * 8> data;
  std::size_t index = 0;
  AnyExpr* alloc() {
    assert(index < data.size());
    return &data[index++];
  }

  UnaryExpr* makeUnary(u32 val) {
    UnaryExpr* expr = &alloc()->unary;
    expr->type = Expr::Unary;
    expr->parent = nullptr;
    expr->val = val;
    return expr;
  }
  BinExpr* makeBinary(ExprOp op, Expr* left, Expr* right) {
    BinExpr* expr = &alloc()->bin;
    expr->type = Expr::Binary;
    expr->op = op;
    expr->parent = nullptr;
    expr->left = left;
    left->parent = expr;
    expr->right = right;
    right->parent = expr;
    return expr;
  };
};

const u32 TevSolverWorkMemSize = sizeof(ExprAlloc);
static_assert(TevSolverWorkMemSizeApprox >= sizeof(ExprAlloc),
              "Invalid workmem size");

static bool optimizeNodeBinary(BinExpr& e) {
  BinExpr* left_bin = e.left->type == Expr::Binary
                          ? reinterpret_cast<BinExpr*>(e.left)
                          : nullptr;
  BinExpr* right_bin = e.right->type == Expr::Binary
                           ? reinterpret_cast<BinExpr*>(e.right)
                           : nullptr;
  UnaryExpr* left_un = e.left->type == Expr::Unary
                           ? reinterpret_cast<UnaryExpr*>(e.left)
                           : nullptr;
  UnaryExpr* right_un = e.right->type == Expr::Unary
                            ? reinterpret_cast<UnaryExpr*>(e.right)
                            : nullptr;

  auto to_unary = [&](u32 type) {
    e.type = Expr::Unary;
    reinterpret_cast<UnaryExpr*>(&e)->val = type;
  };
  auto to_expr = [&](Expr& other) {
    e.type = other.type;
    if (other.type == Expr::Unary) {
      reinterpret_cast<UnaryExpr&>(e).val =
          reinterpret_cast<UnaryExpr&>(other).val;
    } else {
      auto& from = reinterpret_cast<BinExpr&>(other);
      auto& to = reinterpret_cast<BinExpr&>(e);
      to.op = from.op;
      to.left = from.left;
      to.right = from.right;
    }
  };

  const bool left_zero = left_un != nullptr && left_un->val == Zero;
  const bool left_one = left_un != nullptr && left_un->val == One;
  const bool right_zero = right_un != nullptr && right_un->val == Zero;
  const bool right_one = right_un != nullptr && right_un->val == One;

  // (A == A) = true
  if (e.op == ExprOp::Eq) {
    // Does not recursively compare*
    // Only supports unary values*
    if (left_un != nullptr && right_un != nullptr &&
        left_un->val == right_un->val) {
      to_unary(One);
      return true;
    }
  }

  // (A > A) = false
  if (e.op == ExprOp::Gt) {
    // Does not recursively compare*
    // Only supports unary values*
    if (left_un != nullptr && right_un != nullptr &&
        left_un->val == right_un->val) {
      to_unary(Zero);
      return true;
    }
  }

  // (1 > 0) = true
  // (1 > 0.5) = true
  // (0 > 1) = false
  // (0 > 0.5) = false
  std::array<RegEx, 5> ordering{
      Zero,
      Half,
      One,
      // Following two not expected in TEV inputs
      Two,
      Four,
  };
  if (e.op == ExprOp::Gt && left_un != nullptr && right_un != nullptr) {
    auto left_it = std::find(ordering.begin(), ordering.end(), left_un->val);
    auto right_it = std::find(ordering.begin(), ordering.end(), right_un->val);
    if (left_it != ordering.end() && right_it != ordering.end()) {
      to_unary(left_it > right_it ? One : Zero);
      return true;
    }
  }

  // Zero product
  // 0 * A = 0
  // A * 0 = 0
  if (e.op == ExprOp::Mul && (left_zero || right_zero)) {
    to_unary(Zero);
    return true;
  }

  // Multiplicative identity
  // 1 * A = A
  // A * 1 = A
  // (1 * 1 = 1)
  if (e.op == ExprOp::Mul && (left_one || right_one)) {
    if (left_one && right_one) {
      to_unary(One);
      return true;
    }
    if (left_one) {
      to_expr(*e.right);
      return true;
    }
    to_expr(*e.left);
    return true;
  }

  // A - 0 = A + 0
  // (0 - 0 = 0 + 0)
  if (e.op == ExprOp::Sub && right_zero) {
    e.op = ExprOp::Add;
  }

  // Additive identity
  // 0 + A = A
  // A + 0 = A
  // (0 + 0 = 0)
  if (e.op == ExprOp::Add && (left_zero || right_zero)) {
    if (left_zero && right_zero) {
      to_unary(Zero);
      return true;
    }
    if (left_zero) {
      to_expr(*e.right);
      return true;
    }
    to_expr(*e.left);
    return true;
  }

  // 1 - 1 = 0
  if (e.op == ExprOp::Sub && left_one && right_one) {
    to_unary(Zero);
    return true;
  }

  if (e.op == ExprOp::Select) {
    // Logical resolution
    // (A ? B : B) = B
    // Not recursive*
    if (right_bin->left->type == Expr::Unary &&
        right_bin->right->type == Expr::Unary &&
        reinterpret_cast<UnaryExpr*>(right_bin->left)->val ==
            reinterpret_cast<UnaryExpr*>(right_bin->right)->val) {
      to_unary(reinterpret_cast<UnaryExpr*>(right_bin->left)->val);
    }
    // Modus Ponens
    // (true ? A : B) = A
    if (left_one) {
      to_expr(*right_bin->left);
      return true;
    }
    // Disjunctive syllogism
    // (false ? A : B) = B
    if (left_zero) {
      to_expr(*right_bin->right);
      return true;
    }
  }

  // Process children
  bool changed = false;
  if (left_bin)
    changed |= optimizeNodeBinary(*left_bin);
  if (right_bin)
    changed |= optimizeNodeBinary(*right_bin);

  return changed;
};

bool optimizeNode(Expr& e) {
  return e.type == Expr::Binary &&
         optimizeNodeBinary(reinterpret_cast<BinExpr&>(e));
}

static const char* printExpr(UnaryExpr& e) {
  switch (e.val) {
  case A:
    return "A";
  case B:
    return "B";
  case C:
    return "C";
  case D:
    return "D";
  case One:
    return "1";
  case Zero:
    return "0";
  case Half:
    return "0.5";
  case Two:
    return "2";
  case Four:
    return "4";
  default:
    return "?";
  }
}

static void printExpr(BinExpr& e, rsl::StringBuilder& builder,
                      bool root = false) {
  BinExpr* left_bin = e.left->type == Expr::Binary
                          ? reinterpret_cast<BinExpr*>(e.left)
                          : nullptr;
  BinExpr* right_bin = e.right->type == Expr::Binary
                           ? reinterpret_cast<BinExpr*>(e.right)
                           : nullptr;
  UnaryExpr* left_un = e.left->type == Expr::Unary
                           ? reinterpret_cast<UnaryExpr*>(e.left)
                           : nullptr;
  UnaryExpr* right_un = e.right->type == Expr::Unary
                            ? reinterpret_cast<UnaryExpr*>(e.right)
                            : nullptr;
  if (!root)
    builder.append("(");

  if (left_bin != nullptr)
    printExpr(*left_bin, builder);
  else
    builder.append(printExpr(*left_un));

  switch (e.op) {
  case ExprOp::Add:
    builder.append(" + ");
    break;
  case ExprOp::Mul:
    builder.append(" * ");
    break;
  case ExprOp::Sub:
    builder.append(" - ");
    break;
  case ExprOp::Select:
    builder.append(" ? ");
    break;
  case ExprOp::Pair:
    // Assume Pair is a child of a Select*
    builder.append(" : ");
    break;
  case ExprOp::Gt:
    builder.append(" > ");
    break;
  case ExprOp::Eq:
    builder.append(" == ");
    break;
  }

  if (right_bin != nullptr)
    printExpr(*right_bin, builder);
  else
    builder.append(printExpr(*right_un));

  if (!root)
    builder.append(")");
}
void printExprPoly(Expr& e, rsl::StringBuilder& builder, bool root) {
  if (e.type == Expr::Binary)
    printExpr(reinterpret_cast<BinExpr&>(e), builder, root);
  else
    builder.append(printExpr(reinterpret_cast<UnaryExpr&>(e)));

  if (root)
    builder.append("\n");
}
u32 computeUsed(const Expr& e) {
  if (e.type == Expr::Binary) {
    const auto& b = reinterpret_cast<const BinExpr&>(e);
    return computeUsed(*b.left) | computeUsed(*b.right);
  } else {
    const auto& u = reinterpret_cast<const UnaryExpr&>(e);
    return u.val;
  }
}

template <typename T>
static Expr& solve_tev_stage_impl(const T& substage,
                                  rsl::StringBuilder& builder,
                                  bool do_print_inter, u8* workmem,
                                  std::size_t workmem_size) {
  assert(workmem_size >= sizeof(ExprAlloc) &&
         "Not enough memory.. cannot proceed");

  // std::fill_n(workmem, workmem_size, 0);
  ExprAlloc& allocator = *reinterpret_cast<ExprAlloc*>(workmem);

  auto make_unary = [&](u32 val) { return allocator.makeUnary(val); };
  auto make_binary = [&](ExprOp op, Expr* left, Expr* right) {
    return allocator.makeBinary(op, left, right);
  };

  auto arg_to_state = [&](u32 id, auto arg, const auto& substage) -> u32 {
    if constexpr (std::is_same_v<decltype(arg), gx::TevColorArg>) {
      switch (arg) {
      case gx::TevColorArg::zero:
        return Zero;
      case gx::TevColorArg::one:
        return One;
      case gx::TevColorArg::half:
        return Half;
      case gx::TevColorArg::konst:
        if (substage.constantSelection == gx::TevKColorSel::const_8_8)
          return One;
        if (substage.constantSelection == gx::TevKColorSel::const_4_8)
          return Half;
        return id;
      default:
        return id;
      }
    } else if constexpr (std::is_same_v<decltype(arg), gx::TevAlphaArg>) {
      switch (arg) {
      case gx::TevAlphaArg::zero:
        return Zero;
      case gx::TevAlphaArg::konst:
        if (substage.constantSelection == gx::TevKAlphaSel::const_8_8)
          return One;
        if (substage.constantSelection == gx::TevKAlphaSel::const_4_8)
          return Half;
        return id;
      default:
        return id;
      }
    }
    // Unknown type
    return id;
  };

  // D + ((1-C) * A  +  C * B)
  // BinaryOp<+>(UnaryOp(D),
  //   BinaryOp<+>(
  //     BinaryOp<*>(BinaryOp<->(UnaryOp(1), UnaryOp(C)),
  //     UnaryOp(A)), BinaryOp<*>(UnaryOp(C), UnaryOp(B))
  //   )
  // )
  //
  // 11 Ops

  const auto unary_a = make_unary(arg_to_state(A, substage.a, substage));
  const auto unary_b = make_unary(arg_to_state(B, substage.b, substage));
  const auto unary_c = make_unary(arg_to_state(C, substage.c, substage));
  const auto unary_d = make_unary(arg_to_state(D, substage.d, substage));
  librii::gx::TevColorOp_H h(
      static_cast<librii::gx::TevColorOp>(substage.formula));

  BinExpr* root = nullptr;

  if (h.op != librii::gx::TevColorOp_H::Mask) {
    const auto op =
        h.op == librii::gx::TevColorOp_H::Add ? ExprOp::Add : ExprOp::Sub;
    root = make_binary(
        op, unary_d,
        make_binary(
            ExprOp::Add,
            make_binary(ExprOp::Mul,
                        make_binary(ExprOp::Sub, make_unary(One), unary_c),
                        unary_a),
            make_binary(ExprOp::Mul, unary_c, unary_b)));
    // Bias
    if (substage.bias == gx::TevBias::add_half)
      root = make_binary(ExprOp::Add, root, make_unary(Half));
    else if (substage.bias == gx::TevBias::sub_half)
      root = make_binary(ExprOp::Sub, root, make_unary(Half));

    // Scale
    if (substage.scale == gx::TevScale::scale_2)
      root = make_binary(ExprOp::Mul, root, make_unary(Two));
    if (substage.scale == gx::TevScale::scale_4)
      root = make_binary(ExprOp::Mul, root, make_unary(Four));
    if (substage.scale == gx::TevScale::divide_2)
      root = make_binary(ExprOp::Mul, root, make_unary(Half));
  } else {
    const auto op =
        h.maskOp == librii::gx::TevColorOp_H::Gt ? ExprOp::Gt : ExprOp::Eq;
    root = make_binary(
        ExprOp::Add, unary_d,
        make_binary(ExprOp::Select, make_binary(op, unary_a, unary_b),
                    make_binary(ExprOp::Pair, unary_c, make_unary(Zero))));
  }

  // A + C(B-A)
  //
  // BinExpr* root = make_binary(
  //     ExprOp::Add, unary_d,
  //     make_binary(
  //         ExprOp::Add, unary_a,
  //         make_binary(ExprOp::Mul, unary_c,
  //                     make_binary(ExprOp::Sub, unary_b,
  //                     unary_a))));

  auto print_inter = [&]() {
    if (do_print_inter)
      printExprPoly(*root, builder, true);
  };

  print_inter();
  while (optimizeNode(*root)) {
    print_inter();
  }

  return *root;
};

Expr& solveTevStage(const gx::TevStage::ColorStage& substage,
                    rsl::StringBuilder& builder, u8* workmem,
                    std::size_t workmem_size, bool do_print_inter) {

  return solve_tev_stage_impl(substage, builder, do_print_inter, workmem,
                              workmem_size);
}
Expr& solveTevStage(const gx::TevStage::AlphaStage& substage,
                    rsl::StringBuilder& builder, u8* workmem,
                    std::size_t workmem_size, bool do_print_inter) {
  return solve_tev_stage_impl(substage, builder, do_print_inter, workmem,
                              workmem_size);
}

} // namespace impl

} // namespace libcube::UI
