#pragma once

#include <vector>
#include <core/common.h>

#include <core/3d/i3dmodel.hpp>

namespace riistudio::frontend {

class ImagePreview
{
public:
	ImagePreview();
	~ImagePreview();
	void setFromImage(const lib3d::Texture& tex);

	void draw(float width=-1.0f, float height=-1.0f);

private:
	u16 width = 0;
	u16 height = 0;
public:
	std::vector<u8> mDecodeBuf;

	u32 mGpuTexId = 0;
	bool mTexUploaded = false;

	float mScale = 1.0f;
};

}
