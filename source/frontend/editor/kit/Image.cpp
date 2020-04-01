#include <core/3d/gl.hpp>
#include "Image.hpp"
#include <vendor/imgui/imgui.h>

namespace riistudio::frontend {

ImagePreview::ImagePreview()
{
}
ImagePreview::~ImagePreview()
{
	if (mTexUploaded)
	{
		glDeleteTextures(1, &mGpuTexId);
		mTexUploaded = false;
	}
}

void ImagePreview::setFromImage(const lib3d::Texture& tex)
{
	width = tex.getWidth();
	height = tex.getHeight();
	tex.decode(mDecodeBuf, false);

	if (mTexUploaded)
	{
		glDeleteTextures(1, &mGpuTexId);
		mTexUploaded = false;
	}
}

void ImagePreview::draw(float wd, float ht)
{
	if (!mTexUploaded && mDecodeBuf.size() && width && height)
	{
		glGenTextures(1, &mGpuTexId);
		mTexUploaded = true;
	}

	if (!mTexUploaded || !mDecodeBuf.size())
	{
		ImGui::Text("No image to display");
		return;
	}

	ImGui::SliderFloat("Scale", &mScale, 0.0f, 10.0f);

	glBindTexture(GL_TEXTURE_2D, mGpuTexId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mDecodeBuf.data());
	ImGui::Image((void*)(intptr_t)mGpuTexId, ImVec2(
		(wd > 0 ? wd : width) * mScale,
		(ht > 0 ? ht : height) * mScale));
}
}
