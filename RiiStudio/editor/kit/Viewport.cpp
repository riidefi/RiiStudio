#include "Viewport.hpp"
#include <GL/gl3w.h>
#include <imgui/imgui.h>

void Viewport::genFBO(u32 width, u32 height)
{
	if (width == mLastWidth && height == mLastHeight)
		return;

	mLastWidth = width;
	mLastHeight = height;

	destroyFBO();

	glGenFramebuffers(1, &mFboId);
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);

	glGenTextures(1, &mTexColorBufId);
	glBindTexture(GL_TEXTURE_2D, mTexColorBufId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexColorBufId, 0);

	u32 rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	mbFboLoaded = true;
}

void Viewport::destroyFBO()
{
	if (!mbFboLoaded) return;

	glDeleteFramebuffers(1, &mFboId);
	glDeleteTextures(1, &mTexColorBufId);

	mbFboLoaded = false;
}

bool Viewport::begin(u32 width, u32 height)
{
	assert(!mbInRender);
	mbInRender = true;

	genFBO(width, height);

	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);

	return true;
}

void Viewport::end()
{
	assert(mbInRender);
	mbInRender = false;

	ImVec2 max = ImGui::GetContentRegionAvail();
	max.y -= ImGui::GetItemsLineHeightWithSpacing();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ImGui::Image((void*)mTexColorBufId,
		{ static_cast<float>(mLastWidth), static_cast<float>(max.y) },
		{0.0f, 1.0f},
		{1.0f, .0f});
}
