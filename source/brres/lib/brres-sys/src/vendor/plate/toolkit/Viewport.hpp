#pragma once

#include <stdint.h>

namespace plate::tk {

//! The design here is not entirely ImGui style (immediate / stateless), as it
//! is much more efficient to directly hold onto the frame buffer.
//!
class Viewport {
public:
  //! The constructor.
  //!
  Viewport();

  //! The destructor.
  //!
  virtual ~Viewport();

  //! @brief Begin a viewport block.
  //!
  //! All begin() calls must be paired with an end() call. They cannot be
  //! nested.
  //!
  bool begin(unsigned width, unsigned height);

  //! @brief End a viewport block.
  //!
  //! It is invalid to call end() when not immediately followed by begin().
  //!
  void end();

private:
  // Create a new FBO (no operation if width/height match)
  void createFbo(unsigned width, unsigned height);

  // Destroy the last FBO, freeing memory. Not necessary on first begin() or
  // when dimensions match with the last frame.
  // Does nothing if no FBO is loaded.
  void destroyFbo();

  struct Flags {
    // Have the Color Buffer, FBO, and RBO been loaded?
    // (This also implies mLastWidth and mLastHeight are valid)
    bool fboLoaded : 1;
    // (Debug) Are we in a begin() block?
    bool inBegin : 1;
  } mFlags{};

  bool isFboLoaded() const { return mFlags.fboLoaded; }
  void setFboLoaded(bool v) { mFlags.fboLoaded = v; }
  bool isInBegin() const { return mFlags.inBegin; }
  void setInBegin(bool v) { mFlags.inBegin = v; }

  // GL handles
  uint32_t mImageBufId = ~static_cast<uint32_t>(0);
  uint32_t mFboId = ~static_cast<uint32_t>(0);
  uint32_t mRboId = ~static_cast<uint32_t>(0);

public:
  // Remember the last resolution.
  struct ResolutionData {
    unsigned width = 0;
    unsigned height = 0;

    float retina_x = 1.0f;
    float retina_y = 1.0f;

    // Adjust for Retina display
    int computeGlWidth() const {
      return static_cast<int>(static_cast<float>(width) * retina_x);
    }
    // Adjust for Retina display
    int computeGlHeight() const {
      return static_cast<int>(static_cast<float>(height) * retina_y);
    }

    bool operator==(const ResolutionData&) const = default;
  };
  ResolutionData queryResolution(unsigned width, unsigned height) const;

  ResolutionData mLastResolution;
};

} // namespace plate::tk
