#pragma once

#include "Exports.hpp"
#include "SysDolphin/MOD/MOD.hpp"
#include <string>

namespace libcube {
const pl::FileEditor* getMODEditorHandle();

struct MODEditor : public pl::FileEditor, public pl::Readable, public pl::TransformStack, public pl::ITextureList
{
	~MODEditor() override = default;

	// public pl::ITextureList
	u32 getNumTex() const
	{
		if (coreRes.m_textures.empty() != false)
		{
			return coreRes.m_textures.size();
		}
	}
	// public pl::ITextureList
	std::string getNameAt(int idx) const
	{
		return std::string("Texture #") + std::to_string(idx);
	}

	// public pl::Readable
	bool tryRead(oishii::BinaryReader& reader) override
	{
		coreRes.parse(reader);
		return true;
	}

	MODEditor()
	{
		mMagics.push_back(0x00000000);
		mExtensions.push_back(".mod");
		mInterfaces.push_back((pl::TransformStack*)this);
		mInterfaces.push_back((pl::Readable*)this);
		mInterfaces.push_back((pl::ITextureList*)this);
	}

private:
	pikmin1::MOD coreRes;
};

namespace
{
MODEditor __MODEditor;
} // namespace

const pl::FileEditor* getMODEditorHandle()
{
	return static_cast<const pl::FileEditor*>(&__MODEditor);
}

} // namespace libcube
