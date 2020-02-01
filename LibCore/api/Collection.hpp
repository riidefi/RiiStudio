#pragma once

#include <LibCore/common.h>

#include <type_traits>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "Node.hpp"
#include "RichName.hpp"


namespace px {

class CollectionHost : public IDestructable
{
protected:
	struct Collection
	{
		std::string mNodeType; // Folder type restriction
		std::vector<Dynamic> mNodes;
	};
public:
	PX_TYPE_INFO("Collection Host", "collection_host", "CollectionHost");

	CollectionHost(std::vector<std::string> folders)
	{
		for (const auto& folder : folders)
		{
			mEntries.emplace(folder, Collection{ folder, {} });
			mLut.emplace_back(folder);
		}
	}

	class CollectionHandle
	{
	public:
		std::string getType() const { return mCollection.mNodeType; }
		u64 size() const { return mCollection.mNodes.size(); }
		Dynamic& atDynamic(u64 idx) { return mCollection.mNodes[idx]; }
		void* atAs(u64 idx, const std::string& type)
		{
			if (idx >= mCollection.mNodes.size())
				return nullptr;
			auto& node = mCollection.mNodes[idx];
			assert(node.mBase);
			if (node.mType == type)
				return node.mBase;
			std::vector<void*> found;
			ReflectionMesh::getInstance()->findParentOfType(found, node.mBase, node.mType, type);
			
			assert(found.size() <= 1 && "Ambiguous cast");
			return found.size() == 1 ? found[0] : nullptr;
		}
		const void* atAs(u64 idx, const std::string& type) const
		{
			if (idx >= mCollection.mNodes.size())
				return nullptr;
			auto& node = mCollection.mNodes[idx];
			assert(node.mBase);
			if (node.mType == type)
				return node.mBase;
			std::vector<void*> found;
			ReflectionMesh::getInstance()->findParentOfType(found, node.mBase, node.mType, type);

			assert(found.size() <= 1 && "Ambiguous cast");
			return found.size() == 1 ? found[0] : nullptr;
		}
		void push(const std::string& type, void* base, std::unique_ptr<IDestructable> v)
		{
			assert(base);
			assert(v.get());
			mCollection.mNodes.emplace_back(std::move(v), base, type);
		}
		template<typename T>
		void push(std::unique_ptr<T> in)
		{
			assert(in);
			void* data = in.get();
			push(std::string(T::TypeInfo.namespacedId), data, std::move(in));
		}

		template<typename T>
		T* at(u64 idx)
		{
			return reinterpret_cast<T*>(atAs(idx, std::string(T::TypeInfo.namespacedId)));
		}
		template<typename T>
		const T* at(u64 idx) const
		{
			return reinterpret_cast<const T*>(atAs(idx, std::string(T::TypeInfo.namespacedId)));
		}
		CollectionHandle(Collection& c) : mCollection(c) {}
	public:
		Collection& getCollection() { return mCollection; }
		const Collection& getCollection() const { return mCollection; }
	private:
		Collection& mCollection;
	};
	std::optional<CollectionHandle> getFolder(const std::string& type)
	{
		auto found = mEntries.find(type);
		if (found != mEntries.end())
			return CollectionHandle{ (*found).second };

		auto info = ReflectionMesh::getInstance()->lookupInfo(type);
		if (info.getNumParents() == 0)
			return {};

		for (int i = 0; i < info.getNumParents(); ++i)
		{
			auto opt = getFolder(info.getParent(i).getName());
			if (opt.has_value()) return opt;
		}

		return {};
	}
	std::optional<CollectionHandle> getFolder(const std::string& type) const
	{
		const auto found = mEntries.find(type);
		if (found != mEntries.end())
			return CollectionHandle(const_cast<Collection&>((*found).second));

		auto info = ReflectionMesh::getInstance()->lookupInfo(type);
		if (info.getNumParents() == 0)
			return {};

		for (int i = 0; i < info.getNumParents(); ++i)
		{
			auto opt = getFolder(info.getParent(i).getName());
			if (opt.has_value()) return opt;
		}

		return {};
	}
	template<typename T>
	std::optional<CollectionHandle> getFolder()
	{
		return getFolder(std::string(T::TypeInfo.namespacedId));
	}
	template<typename T>
	std::optional<CollectionHandle> getFolder() const
	{
		return getFolder(std::string(T::TypeInfo.namespacedId));
	}

	u32 getNumFolders()
	{
		return (u32)mEntries.size();
	}
	CollectionHandle getFolder(u32 idx)
	{
		return mEntries[mLut[idx]];
	}

private:
	// Collection type : Entries
	std::map<std::string, Collection> mEntries;
	std::vector<std::string> mLut;
};

}
