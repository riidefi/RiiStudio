#pragma once

#include "Exports.hpp"
#include "SysDolphin/TXE/TXE.hpp"
#include <string>

namespace libcube {
const pl::FileEditor* getTXEEditorHandle();

// Static initialization unfortunate

struct TXEEditor : public pl::FileEditor, public pl::Readable, public pl::ITextureList, public pl::TransformStack
{
	~TXEEditor() override = default;
	// public pl::TransformStack
	struct GenStats : public pl::TransformStack::XForm
	{
		GenStats()
		{
			name = pl::RichName{ "Generate Statistics", "GenStats", "stats" };
			mParams.push_back(pl::TransformStack::Param{
				pl::RichName{"Verbose", "verbose", "verbose"},
				pl::TransformStack::ParamType::Flag,
				{} });
		}

		void perform() override
		{
			printf("TODO: Generate stats!\n");
		}
	};

	// public pl::Readable
	bool tryRead(oishii::BinaryReader& reader) override
	{
		reader.dispatch<pikmin1::TXE>(coreRes);
		return true;
	}
	// public pl::ITextureList
	u32 getNumTex() const override
	{
		return 1; // there can only be 1 texture in the texture file
	}
	std::string getNameAt(int idx) const override
	{
		return std::string("Texture #") + std::to_string(idx);
	}

	TXEEditor()
	{
		mMagics.push_back(0x00000001); // there are no magics for this file format
		mExtensions.push_back(".txe");
		mInterfaces.push_back((pl::TransformStack*)this);
		mInterfaces.push_back((pl::Readable*)this);
		mInterfaces.push_back((pl::ITextureList*)this);
	}

private:
	pikmin1::TXE coreRes;
};

namespace
{
TXEEditor __TXEEditor;
} // namespace

const pl::FileEditor* getTXEEditorHandle()
{
	return static_cast<const pl::FileEditor*>(&__TXEEditor);
}

} // namespace libcube

