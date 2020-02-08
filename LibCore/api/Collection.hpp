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

#include <LibCore/windows/SelectionManager.hpp>

namespace px {

template<typename T>
struct ConcreteCollectionHandle;

class CollectionHost : public IDestructable
{
protected:
	struct Collection
	{
		std::string mNodeType; // Folder type restriction
		std::vector<Dynamic> mNodes;

		std::vector<std::size_t> mSelected;
		std::size_t mActiveSelect = 0;
	};
public:
	PX_TYPE_INFO("Collection Host", "collection_host", "CollectionHost");

	CollectionHost(const std::vector<std::string>& folders)
	{
		for (const auto& folder : folders)
		{
			mEntries.emplace(folder, Collection{ folder, {} });
			mLut.emplace_back(folder);
		}
	}

#define PX_COLLECTION_FOLDER_GETTER(name, type) \
	px::ConcreteCollectionHandle<type> name() { assert(getFolder<type>().has_value()); \
		return px::ConcreteCollectionHandle<type>(getFolder<type>().value()); } \
	const px::ConcreteCollectionHandle<type> name() const { assert(getFolder<type>().has_value()); \
		return px::ConcreteCollectionHandle<type>(getFolder<type>().value()); }

	class CollectionHandle
	{
	public:
		std::string getType() const { return mCollection.mNodeType; }
		u64 size() const { return mCollection.mNodes.size(); }
		Dynamic& atDynamic(u64 idx) { return mCollection.mNodes[idx]; }
		std::string nameAt(u64 idx) const
		{
			if (idx >= mCollection.mNodes.size())
				return "Out of bounds";

			return mCollection.mNodes[idx].mOwner->getName();
		}
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

		bool isSelected(std::size_t index) const
		{
			const auto& vec = mCollection.mSelected;
			return std::find(vec.begin(), vec.end(), index) != vec.end();
		}
		bool select(std::size_t index)
		{
			if (isSelected(index)) return true;

			mCollection.mSelected.push_back(index);
			return false;
		}
		bool deselect(std::size_t index)
		{
			auto it = std::find(mCollection.mSelected.begin(), mCollection.mSelected.end(), index);

			if (it == mCollection.mSelected.end()) return false;
			mCollection.mSelected.erase(it);
			return true;
		}
		std::size_t clearSelection()
		{
			std::size_t before = mCollection.mSelected.size();
			mCollection.mSelected.clear();
			return before;
		}
		std::size_t getActiveSelection() const
		{
			return mCollection.mActiveSelect;
		}
		std::size_t setActiveSelection(std::size_t value)
		{
			const std::size_t old = mCollection.mActiveSelect;
			mCollection.mActiveSelect = value;
			return old;
		}

		CollectionHandle(Collection& c) : mCollection(c) {}
	public:
		Collection& getCollection() { return mCollection; }
		const Collection& getCollection() const { return mCollection; }
	private:
		Collection& mCollection;
	};


	std::optional<CollectionHandle> getFolder(const std::string& type, bool fromParent=false, bool fromChild=false) const
	{
		const auto found = mEntries.find(type);
		if (found != mEntries.end())
			return CollectionHandle(const_cast<Collection&>((*found).second));

		auto info = ReflectionMesh::getInstance()->lookupInfo(type);

		if (!fromChild)
		{
			for (int i = 0; i < info.getNumParents(); ++i)
			{
				assert(info.getParent(i).getName() != info.getName());

				auto opt = getFolder(info.getParent(i).getName(), true, false);
				if (opt.has_value()) return opt;
			}
		}

		if (!fromParent)
		{
			// If the folder is of a more specialized type
			for (int i = 0; i < info.getNumChildren(); ++i)
			{
				const std::string childName = info.getChild(i).getName();
				const std::string infName = info.getName();
				assert(childName != infName);

				auto opt = getFolder(childName, false, true);
				if (opt.has_value()) return opt;
			}
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
template<typename T>
struct ConcreteCollectionHandle : public CollectionHost::CollectionHandle
{
	ConcreteCollectionHandle(CollectionHandle&& c)
		: CollectionHandle(c.getCollection())
	{
	}

	T& operator[] (std::size_t idx) { return *at<T>(idx); }
	const T& operator[] (std::size_t idx) const { return *at<T>(idx); }
};

}
