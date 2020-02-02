#pragma once

#include <vector>
#include <LibCore/common.h>

#include <Lib3D/interface/i3dmodel.hpp>

class ImagePreview
{
public:
    ImagePreview();

    void setFromImage(const lib3d::Texture& tex);

    void draw();

private:
    u16 width = 0;
    u16 height = 0;
    std::vector<u8> mDecodeBuf;

    u32 mGpuTexId = 0;
    bool mTexUploaded = false;
};