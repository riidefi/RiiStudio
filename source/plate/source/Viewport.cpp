#include <assert.h>
#include <imgui/imgui.h>
#include <plate/gl.hpp>
#include <plate/toolkit/Viewport.hpp>

namespace plate::tk {

Viewport::Viewport() {}
Viewport::~Viewport() { destroyFbo(); }

bool Viewport::begin(unsigned width, unsigned height) {
  assert(!isInBegin());
  if (isInBegin())
    return false;
  setInBegin(true);

  createFbo(width, height);

  glViewport(0, 0, width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, mFboId);

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
  const auto vert_ratio = region.x / static_cast<float>(mLastWidth);
  const auto horiz_ratio = region.y / static_cast<float>(mLastHeight);

  // TODO
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  ImGui::Image(reinterpret_cast<void*>(mImageBufId),
               {static_cast<float>(region.x), static_cast<float>(region.y)},
               {0.0f, horiz_ratio}, {vert_ratio, .0f});
}

void Viewport::createFbo(unsigned width, unsigned height) {
  if (width == mLastWidth && height == mLastHeight)
    return;

  mLastWidth = width;
  mLastHeight = height;

  destroyFbo();

  glGenFramebuffers(1, &mFboId);
  glBindFramebuffer(GL_FRAMEBUFFER, mFboId);

  glGenTextures(1, &mImageBufId);
  glBindTexture(GL_TEXTURE_2D, mImageBufId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         mImageBufId, 0);

  glGenRenderbuffers(1, &mRboId);
  glBindRenderbuffer(GL_RENDERBUFFER, mRboId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, mRboId);

  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  setFboLoaded(true);
}

void Viewport::destroyFbo() {
  if (!isFboLoaded())
    return;

  glDeleteFramebuffers(1, &mFboId);
  glDeleteTextures(1, &mImageBufId);
  glDeleteRenderbuffers(1, &mRboId);

  setFboLoaded(false);
}

} // namespace plate::tk
