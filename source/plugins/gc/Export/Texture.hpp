#pragma once

#include <core/3d/i3dmodel.hpp>
#include <vendor/ogc/texture.h>
#include <vendor/dolemu/TextureDecoder/TextureDecoder.h>

#include <vendor/mp/Metaphrasis.h>

namespace libcube {


struct Texture : public riistudio::lib3d::Texture
{
	// PX_TYPE_INFO("GameCube Texture", "gc_tex", "GC::Texture");
	
	inline u32 getEncodedSize(bool mip) const override
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
	virtual void setTextureFormat(u32 format) = 0;
    virtual u32 getMipmapCount() const = 0;
    virtual const u8* getData() const = 0;
	virtual u8* getData() = 0;
	virtual void resizeData() = 0;
    virtual const u8* getPaletteData() const = 0;
    virtual u32 getPaletteFormat() const = 0;

	//! @brief Set the image encoder based on the expression profile. Pixels are not recomputed immediately.
	//!
	//! @param[in] optimizeForSize	If the texture should prefer filesize over quality when deciding an encoder.
	//! @param[in] color			If the texture is not grayscale.
	//! @param[in] occlusion		The pixel occlusion selection.
	//!
	void setEncoder(bool optimizeForSize, bool color, Occlusion occlusion) override
	{
		// TODO
		setTextureFormat(6);
	}

	//! @brief Encode the texture based on the current encoder, width, height, etc. and supplied raw data. 
	//!
	//! @param[in] rawRGBA
	//!				- Raw pointer to a RGBA32 pixel array.
	//!				- Must be sized width * height * 4.
	//!				- If mipmaps are configured, this must also include all additional mip levels.
	//!
	void encode(const u8* rawRGBA) override
	{
		assert(getEncodedSize(false) == getDecodedSize(false));

		resizeData();

		auto data = convertBufferToRGBA8((uint32_t*)rawRGBA, getWidth(), getHeight());
		memcpy(getData(), data.get(), getEncodedSize(false));
	}
};

} // namespace libcube
