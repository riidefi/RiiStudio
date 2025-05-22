#include <assert.h>
#include <imgui/imgui.h>
#include <plate/gl.hpp>
#include <plate/toolkit/Viewport.hpp>

namespace plate::tk {

Viewport::Viewport() = default;
Viewport::~Viewport() { destroyFbo(); }

bool Viewport::begin(unsigned width, unsigned height) {
  assert(!isInBegin());
  if (isInBegin())
    return false;
  setInBegin(true);

  createFbo(width, height);

#ifdef RII_GL
  // Retina scaling: Convert width to pixels
  const ResolutionData cur_resolution = queryResolution(width, height);

  glViewport(0, 0, cur_resolution.computeGlWidth(),
             cur_resolution.computeGlHeight());
  glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
#endif

  return true;
}

void Viewport::end() {
  assert(isInBegin());
  if (!isInBegin())
    return;
  setInBegin(false);

  // We do these calculations now, as the user may have used ImGui within the
  // viewport, making our actual region smaller than our framebuffer.
  const auto region = ImGui::GetContentRegionAvail();
  const auto vert_ratio = region.x / static_cast<float>(mLastResolution.width);
  const auto horiz_ratio =
      region.y / static_cast<float>(mLastResolution.height);

// TODO
#ifdef RII_GL
  // Clear alpha channel
  glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
  glColorMask(/*r*/ GL_FALSE, /*g*/ GL_FALSE, /*b*/ GL_FALSE, /*a*/ GL_TRUE);
  glClearColor(0, 0, 0, 0xFF);
  glClear(GL_COLOR_BUFFER_BIT);
  // Unbind
  glColorMask(/*r*/ GL_TRUE, /*g*/ GL_TRUE, /*b*/ GL_TRUE, /*a*/ GL_TRUE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  ImGui::Image(
      reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(mImageBufId)),
               {static_cast<float>(region.x), static_cast<float>(region.y)},
               {0.0f, horiz_ratio}, {vert_ratio, .0f});
#endif
}

void Viewport::createFbo(unsigned width, unsigned height) {
  const ResolutionData cur_resolution = queryResolution(width, height);
  if (mLastResolution == cur_resolution)
    return;
  mLastResolution = cur_resolution;

  destroyFbo();

#ifdef RII_GL
  glGenFramebuffers(1, &mFboId);
  glBindFramebuffer(GL_FRAMEBUFFER, mFboId);

  glGenTextures(1, &mImageBufId);
  glBindTexture(GL_TEXTURE_2D, mImageBufId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cur_resolution.computeGlWidth(),
               cur_resolution.computeGlHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         mImageBufId, 0);

  glGenRenderbuffers(1, &mRboId);
  glBindRenderbuffer(GL_RENDERBUFFER, mRboId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                        cur_resolution.computeGlWidth(),
                        cur_resolution.computeGlHeight());
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, mRboId);

  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  setFboLoaded(true);
#endif
}

void Viewport::destroyFbo() {
  if (!isFboLoaded())
    return;

#ifdef RII_GL
  glDeleteFramebuffers(1, &mFboId);
  glDeleteTextures(1, &mImageBufId);
  glDeleteRenderbuffers(1, &mRboId);
#endif

  setFboLoaded(false);
}

Viewport::ResolutionData Viewport::queryResolution(unsigned width,
                                                   unsigned height) const {
  const auto& io = ImGui::GetIO();

  return {.width = width,
          .height = height,
          .retina_x = io.DisplayFramebufferScale.x,
          .retina_y = io.DisplayFramebufferScale.y};
}

} // namespace plate::tk
