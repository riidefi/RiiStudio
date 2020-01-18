#pragma once

#include <map>
#include <vector>
#include <memory>
#include "LibRiiEditor/pluginapi/RichName.hpp"
#include "LibRiiEditor/pluginapi/Interface.hpp"
#include "LibRiiEditor/common.hpp"

namespace pl {

struct TransformStack
{
	TransformStack() = default;
	virtual ~TransformStack() = default;

	enum class ParamType
	{
		Flag, // No arguments
		String, // Simple string
		Enum, // Enumeration of values
	};

	struct EnumOptions
	{
		
	};

	struct Param
	{
		RichName name;
		ParamType type;
		// Only for bool
		bool defaultBool;
		// Only for string
		std::string defaultString;
		// Only for enum
		std::vector<RichName> enumeration;
		int defaultEnum;

		explicit Param(const RichName& n, bool b)
			: name(n), type(ParamType::Flag), defaultBool(b)
		{}
		explicit Param(const RichName& n, const char* v)
			: name(n), type(ParamType::String), defaultString(v)
		{}
		explicit Param(const RichName& n, std::vector<RichName> en, int def)
			: name(n), type(ParamType::Enum), enumeration(en), defaultEnum(def)
		{}
	};
	struct XFormContext;
	// TODO -- we can do a lot better here..
	struct XForm
	{
		virtual ~XForm() = default;

		virtual void perform(const XFormContext& ctx) const {}

		const Param* getParam(const std::string& id) const
		{
			for (const auto& s : mParams)
				if (s.name.namespacedId == id)
					return &s;
			return nullptr;
		}

		RichName name;
		std::vector<Param> mParams;

		XForm() {}
		XForm(RichName n)
			: name(n)
		{}
		XForm(RichName n, std::vector<Param> p)
			: name(n), mParams(p)
		{}

		template<typename... T>
		void addParam(T... p)
		{
			mParams.push_back(Param(p...));
		}
	};
	struct XFormContext
	{
		std::string getString(const std::string& v) const
		{
			if (mValues.find(v) != mValues.end())
				return mValues.at(v).strVal;
			const Param* p = mCfg.getParam(v);
			assert(p);
			return p ? p->defaultString : "";
		}
		bool getFlag(const std::string& v) const
		{
			if (mValues.find(v) != mValues.end())
				return mValues.at(v).flagVal;
			const Param* p = mCfg.getParam(v);
			assert(p);
			return p ? p->defaultBool : false;
		}
		int getEnum(const std::string& v) const
		{
			if (mValues.find(v) != mValues.end())
				return mValues.at(v).enumVal;
			const Param* p = mCfg.getParam(v);
			assert(p);
			return p ? p->defaultEnum : 0;
		}
		const char* getEnumExposedCStr(const std::string& v) const
		{
			const auto* param = mCfg.getParam(v);
			assert(param);
			if (!param) return "";
			const auto e = getEnum(v);
			assert(e >= 0);
			if (e >= 0) return param->enumeration[e].exposedName.c_str();

			return param->enumeration[param->defaultEnum].exposedName.c_str();
		
		}
		void setString(const std::string& v, const std::string& s)
		{
			mValues[v].strVal = s;
		}
		void setFlag(const std::string& v, bool b)
		{
			mValues[v].flagVal = b;
		}
		void setEnum(const std::string& v, int e)
		{
			mValues[v].enumVal = e;
		}
		XFormContext(const XForm& x)
			: mCfg(x)
		{
			//	for (const auto& it : x.mParams)
			//	{
			//		switch (it.type)
			//		{
			//		}
			//	}
		}
		~XFormContext() = default;
	private:
		struct Val
		{
			union
			{
				int  enumVal;
				bool flagVal;
			};
			std::string strVal;
		};
		std::map<std::string, Val> mValues;
	public:
		const XForm& mCfg;
	};

	// Must exist inside this object -- never freed!
	std::vector<std::unique_ptr<XForm>> mStack;
};

} // namespace pl
