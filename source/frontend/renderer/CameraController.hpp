#pragma once

#include <core/common.h>
#include <frontend/renderer/Camera.hpp>
#include <frontend/renderer/InputState.hpp>
#include <glm/vec2.hpp>
#include <librii/math/aabb.hpp>
#include <optional>

namespace riistudio::frontend {

class CameraController {
public:
  enum class ControllerType : int {
    WASD_Minecraft, //!< First-person controls matching the game Minecraft's
                    //!< flying mode:
                    //!< - WASD inputs only move the camera on the XZ plane.
                    //!< - Manipulating the camera along the Y axis is done
                    //!<   solely through Up/Down inputs (SPACE/SHIFT). These
                    //!<   inputs are global up, and global down, and are
                    //!<   independent of the camera's current attitude.

    WASD_Plane //!< First-person controls fitting a flight simulator.
               //!< - WASD inputs move the camera in XYZ based on its direction,
               //!<   as an actual aircraft would move when pointed in a
               //!<   trajectory.
               //!< - Up/Down inputs are still global, though, as being
               //!<   perpendicular to the view direction can be confusing.
               //!<   (Matching WASD_Minecraft's behavior for SPACE/SHIFT)
  };

  void move(float time_step, InputState input);

  void drawOptions();
  void drawControllerTypeOption();

  Camera mCamera;
  float mMouseSpeed = 0.2f;
  float mSpeed = 150.0f;
  float mSpeedFactor = 0.0f;
  float mHorizontalAngle = 3.14f;
  float mVerticalAngle = 0.0f;
  float mPrevX = 0.0f;
  float mPrevY = 0.0f;
  ControllerType combo_choice_cam = ControllerType::WASD_Minecraft;
};

inline void ConfigureCameraControllerByBounds(CameraController& controller,
                                              const librii::math::AABB& bound) {
  const f32 dist = glm::distance(bound.min, bound.max);
  controller.mSpeedFactor = dist == 0.0f ? 50.0f : dist / 1000.0f;

  if (controller.mCamera.getPosition() == glm::vec3{0.0f}) {
    const auto min = bound.min;
    const auto max = bound.max;

    controller.mCamera.setPosition((min + max) / 2.0f);

    if (min != max) {
      const auto dist = glm::distance(min, max);
      const auto range = 100000.0f;
      const auto expansion = 5.0f; // Let's assume the user wants to be at least
                                   // 5x away from the model.
      controller.mCamera.setClipPlanes(dist / range * expansion,
                                       dist * expansion);
    }
  }
}

} // namespace riistudio::frontend
