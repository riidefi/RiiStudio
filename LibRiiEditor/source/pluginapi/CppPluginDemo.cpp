#include "C_Plugin.h"
#include <vector>
#include <memory>

struct CppInterface : public RXInterface
{
	virtual ~CppInterface() = default;
	virtual void update() = 0;
};

template<typename T>
void setRXArray(RXArray& ar, const std::vector<T>& vec)
{
	ar.mpEntries = (const void**)vec.data();
	ar.nEntries = vec.size();
}


struct CppITransformStack : CppInterface
{
	struct XFormStack : public RXITransformStack
	{
		XFormStack()
		{
			mTransforms.mpEntries = nullptr;
			mTransforms.nEntries = 0;
		}
	};

	CppITransformStack()
	{
		mId = "core::transform_stack";
		body = (void*)&mWrappedStack;
	}

	void update() override
	{
		mStackCache.clear();
		for (const auto& ptr : mStack)
		{
			mStackCache.emplace_back(static_cast<RXITransform*>(ptr.get()));
		}
		//setRXArray<RXITransform*>(mWrappedStack.mTransforms, mStackCache);
	}

	
	std::vector<std::unique_ptr<RXITransform>> mStack;

private:
	RXITransformStack mWrappedStack;
	std::vector<RXITransform*> mStackCache;
};

struct CppDemoPluginBase
{
public:


	void initializeRegistration(RXFileEditor& reg)
	{
		//setRXArray<const char*>(reg.mExtensions, extensions);
		//setRXArray<u32>(reg.mMagics, magics);
		interface_cache.clear();
		for (const auto& ptr : interfaces)
		{
			ptr->update();
			interface_cache.emplace_back(static_cast<RXInterface*>(ptr.get()));
		}
		//setRXArray<RXInterface*>(reg.mInterfaces, interface_cache);
	}
	void initializeRegistration()
	{
		initializeRegistration(mRegistration);
	}
	std::vector<const char*> extensions;
	std::vector<u32> magics;
	std::vector<std::unique_ptr<CppInterface>> interfaces;

	RXFileEditor mRegistration;

private:
	std::vector<RXInterface*> interface_cache;
};

struct CppDemoPackage : public RXPackage
{
	CppDemoPackage(const char* package_name, const char* package_namespace)
	{
		mPackageName = package_name;
		mPackageNamespace = package_namespace;
	}

	void initializeRegistration()
	{

	}
};

struct OurDemo : public CppDemoPluginBase
{
	OurDemo()
	{
		extensions.emplace_back(".cppDemo");
		magics.emplace_back('TST1');
		interfaces.push_back(std::make_unique<CppITransformStack>());

		initializeRegistration();
	}
};

OurDemo CppDemo;
CppDemoPackage DemoPackage("C++ Demo Package", "cppdemo");
