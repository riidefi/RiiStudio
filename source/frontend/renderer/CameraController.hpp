#pragma once

#include <frontend/renderer/Camera.hpp>
#include <frontend/renderer/InputState.hpp>
#include <glm/vec2.hpp>
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

  void move(float time_step, ControllerType controller_type, InputState input);

  void drawOptions();

  Camera mCamera;
  float mMouseSpeed = 0.2f;
  float mSpeed = 150.0f;
  float mSpeedFactor = 0.0f;
  float mHorizontalAngle = 3.14f;
  float mVerticalAngle = 0.0f;
  float mPrevX = 0.0f;
  float mPrevY = 0.0f;
};

} // namespace riistudio::frontend
