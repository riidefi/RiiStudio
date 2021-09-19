#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imgui/imgui.h>

namespace riistudio::frontend {

void Camera::calcMatrices(float width, float height, glm::mat4& projMtx,
                          glm::mat4& viewMtx) {
  const float aspect_ratio =
      static_cast<float>(width) / static_cast<float>(height);

  if (mProjection == Projection::Orthographic) {
    // TODO: Calculate focal plane
    const auto scale = mFOV * 100.0f;

    const float ortho_x = scale * aspect_ratio;
    const float ortho_y = scale;
    projMtx = glm::ortho(-ortho_x, ortho_x, -ortho_y, ortho_y,
                         -(mClipMin + mClipMax), mClipMin + mClipMax);
  } else {
    projMtx =
        glm::perspective(glm::radians(mFOV), aspect_ratio, mClipMin, mClipMax);
  }
  viewMtx = glm::lookAt(mEye, mEye + mDirection, getUp());
}

} // namespace riistudio::frontend
