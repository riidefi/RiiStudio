#pragma once

#include "Exports.hpp"
#include "TPL/TPL.hpp"
#include <string>

namespace libcube {

// Static initialization unfortunate

struct TPLEditor : public pl::FileEditor, public pl::Readable, public pl::ITextureList, public pl::TransformStack
{
	// public pl::TransformStack
	struct GenStats : public pl::TransformStack::XForm
	{
		GenStats()
		{
			name = pl::RichName{"Generate Statistics", "GenStats", "stats"};
			mParams.push_back(pl::TransformStack::Param{
				pl::RichName{"Verbose", "verbose", "verbose"},
				pl::TransformStack::ParamType::Flag,
				{}});
		}

		void perform() override
		{
			printf("TODO: Generate stats!\n");
		}
	};

	// public pl::Readable
	bool tryRead(oishii::BinaryReader& reader, FileEditor& ctx) override
	{
		// Specialize context
		TPLEditor& editor = static_cast<TPLEditor&>(ctx);

		reader.dispatch<DolphinTPL>(editor.coreRes);
		return true;
	}
	// public pl::ITextureList
	u32 getNumTex(const pl::FileEditor& ctx) const override
	{
		return coreRes.mTextures.size();
	}
	std::string getNameAt(const FileEditor& ctx, int idx) const override
	{
		const TPLEditor& editor = static_cast<const TPLEditor&>(ctx);

		return std::string("Texture #") + std::to_string(idx);
	}
	
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

static pl::Package makePackage()
{
	pl::Package pack
	{
		// Package name
		{
			"LibCube",
			"libcube",
			"gc"
		},
		// Editors
		{}
	};

	pack.mEditors.push_back(std::make_unique<TPLEditor>());

	return pack;
}

pl::Package PluginPackage = makePackage();


} // namespace libcube
