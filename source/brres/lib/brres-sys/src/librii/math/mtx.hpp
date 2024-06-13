#pragma once

#include <cmath>
#include <glm/ext/matrix_double4x3.hpp>
#include <glm/mat4x3.hpp>

namespace librii::math {

// Compute the Kullback-Leibler divergence between p and q
static inline double kl_divergence(const glm::highp_dmat4x3& p,
                                   const glm::highp_dmat4x3& q) {
  double divergence = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
      double pi = p[i][j];
      double qi = q[i][j];
      if (pi == 0.0 || qi == 0.0)
        continue;
      divergence += pi * std::log(pi / qi);
    }
  }
  return divergence;
}

static inline double jensen_shannon_divergence(const glm::mat4x3& lowp_p,
                                               const glm::mat4x3& lowp_q) {
  auto p = glm::highp_dmat4x3(lowp_p);
  auto q = glm::highp_dmat4x3(lowp_q);

  // Calculate average distribution
  auto m = (p + q) / 2.0;

  // Calculate KL divergence between p and m
  auto kl1 = kl_divergence(p, m);

  // Calculate KL divergence between q and m
  auto kl2 = kl_divergence(q, m);

  // Calculate JS divergence
  return (kl1 + kl2) / 2.0;
}

} // namespace librii::math
