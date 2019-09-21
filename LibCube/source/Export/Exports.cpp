#pragma once

#include "Exports.hpp"
#include "TPL/TPL.hpp"
#include <string>

namespace libcube {

// Static initialization unfortunate

struct TPLEditor : public pl::FileEditor
{
	struct TODOInterface : public pl::TransformStack
	{
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

		TODOInterface()
		{
			//mStack.emplace_back(XForm)
		}


	} xFormStack;

	struct ReadTPL : public pl::Readable
	{
		bool tryRead(oishii::BinaryReader& reader, FileEditor& ctx) override
		{
			// Specialize context
			TPLEditor& editor = static_cast<TPLEditor&>(ctx);

			reader.dispatch<DolphinTPL>(editor.coreRes);
			return true;
		}

	} readTpl;

	struct TextureList : public pl::ITextureList
	{
		u32 getNumTex(const pl::FileEditor& ctx) const override
		{
			const TPLEditor& editor = static_cast<const TPLEditor&>(ctx);

			return editor.coreRes.mTextures.size();
		}
		std::string getNameAt(const FileEditor& ctx, int idx) const override
		{
			const TPLEditor& editor = static_cast<const TPLEditor&>(ctx);

			return std::string("Texture #") + std::to_string(idx);
		}
	} texList;

	TPLEditor()
	{
		mMagics.push_back(0x0020AF30);
		mExtensions.push_back(".tpl");
		mInterfaces.push_back(&xFormStack);
		mInterfaces.push_back(&readTpl);
		mInterfaces.push_back(&texList);
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
