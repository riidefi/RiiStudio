#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imgui/imgui.h>

namespace riistudio::frontend {

void Camera::calcMatrices(float width, float height, glm::mat4& projMtx,
                          glm::mat4& viewMtx) {
  const float aspect_ratio =
      static_cast<float>(width) / static_cast<float>(height);

  projMtx =
      glm::perspective(glm::radians(mFOV), aspect_ratio, mClipMin, mClipMax);
  viewMtx = glm::lookAt(mEye, mEye + mDirection, getUp());
}

} // namespace riistudio::frontend
