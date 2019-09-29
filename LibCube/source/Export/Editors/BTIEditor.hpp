#pragma once

#include "Exports.hpp"
#include "JSystem/J3D/BTI/BTI.hpp"
#include <string>

namespace libcube {
const pl::FileEditor* getBTIEditorHandle();

// Static initialization unfortunate

struct BTIEditor : public pl::FileEditor, public pl::Readable, public pl::ITextureList, public pl::TransformStack
{
	~BTIEditor() override = default;
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
		reader.dispatch<pikmin1::BTI>(coreRes);
		return true;
	}
	// public pl::ITextureList
	u32 getNumTex() const override
	{
		return coreRes.m_nImage;
	}
	std::string getNameAt(int idx) const override
	{
		return std::string("Texture #") + std::to_string(idx);
	}
	//	std::unique_ptr<FileEditor> cloneFileEditor() override
	//	{
	//		return std::make_unique<FileEditor>(*this);
	//	}

	BTIEditor()
	{
		mMagics.push_back(0x00000002);
		mExtensions.push_back(".bti");
		mInterfaces.push_back((pl::TransformStack*)this);
		mInterfaces.push_back((pl::Readable*)this);
		mInterfaces.push_back((pl::ITextureList*)this);
	}

private:
	pikmin1::BTI coreRes;
};

namespace
{
BTIEditor __BTIEditor;
} // namespace

const pl::FileEditor* getBTIEditorHandle()
{
	return static_cast<const pl::FileEditor*>(&__BTIEditor);
}

} // namespace libcube

