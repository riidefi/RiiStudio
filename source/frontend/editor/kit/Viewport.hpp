#pragma once

#include <core/common.h>

class Viewport
{
public:
	bool begin(u32 width, u32 height);
	void end();

	~Viewport() { destroyFBO(); }

private:
	void genFBO(u32 width, u32 height);
	void destroyFBO();

	bool mbFboLoaded = false;
	bool mbInRender = false;

	u32 mTexColorBufId = 0;
	u32 mFboId = 0;
	u32 rbo = 0;

	// Internal resolution (begin()'s width/height)
	u32 mLastWidth = 1920 / 2;
	u32 mLastHeight = 1080 / 2;
};
