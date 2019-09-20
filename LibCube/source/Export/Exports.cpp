#pragma once

#include "Exports.hpp"
#include "TPL/TPL.hpp"

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
	/*
	ITextureList:
		getNum()
		getNameAt()
		getCommonGCTexureParamAt()
			-> Decoding

	*/

	TPLEditor()
	{
		mMagics.push_back(0x0020AF30);
		mExtensions.push_back(".tpl");
		mInterfaces.push_back(&xFormStack);
		mInterfaces.push_back(&readTpl);	
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
