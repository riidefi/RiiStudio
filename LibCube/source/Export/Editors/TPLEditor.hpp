#pragma once

#include "Exports.hpp"
#include "LibDolphin/TPL/TPL.hpp"
#include <string>

namespace libcube {
const pl::FileEditor* getTPLEditorHandle();

// Static initialization unfortunate

struct TPLEditor : public pl::FileEditor, public pl::Readable, public pl::ITextureList, public pl::TransformStack
{
	~TPLEditor() override = default;
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
		reader.dispatch<DolphinTPL>(coreRes);
		return true;
	}
	// public pl::ITextureList
	u32 getNumTex() const override
	{
		return coreRes.mTextures.size();
	}
	std::string getNameAt(int idx) const override
	{
		return std::string("Texture #") + std::to_string(idx);
	}
	//	std::unique_ptr<FileEditor> cloneFileEditor() override
	//	{
	//		return std::make_unique<FileEditor>(*this);
	//	}

	TPLEditor()
	{
		mMagics.push_back(0x0020AF30);
		mExtensions.push_back(".tpl");
		mInterfaces.push_back((pl::TransformStack*)this);
		mInterfaces.push_back((pl::Readable*)this);
		mInterfaces.push_back((pl::ITextureList*)this);
	}

private:
	DolphinTPL coreRes;
};

namespace
{
TPLEditor __TPLEditor;
} // namespace

const pl::FileEditor* getTPLEditorHandle()
{
	return static_cast<const pl::FileEditor*>(&__TPLEditor);
}

} // namespace libcube

