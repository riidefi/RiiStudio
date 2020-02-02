#pragma once

#include <Lib3D/interface/i3dmodel.hpp>
#include <ThirdParty/ogc/texture.h>
#include <ThirdParty/dolemu/TextureDecoder/TextureDecoder.h>

namespace libcube {

struct Texture : public lib3d::Texture
{
	PX_TYPE_INFO("GameCube Texture", "gc_tex", "GC::Texture");
	
	inline u32 getDecodedSize(bool mip) const override
    {
        return GetTexBufferSize(getWidth(), getHeight(), getTextureFormat(),
            mip && getMipmapCount() > 1, mip ? getMipmapCount() + 1 : 0);
    }
	inline void decode(std::vector<u8>& out, bool mip) const override
    {
        const u32 size = getDecodedSize(mip);

        if (out.size() < size)
            out.resize(size);

        TexDecoder_Decode(out.data(), getData(), getWidth(), getHeight(),
            (TextureFormat)getTextureFormat(), getPaletteData(), (TLUTFormat)getPaletteFormat());
    }

    virtual u32 getTextureFormat() const = 0;
    virtual u32 getMipmapCount() const = 0;
    virtual const u8* getData() const = 0;

    virtual const u8* getPaletteData() const = 0;
    virtual u32 getPaletteFormat() const = 0;
};

} // namespace libcube
