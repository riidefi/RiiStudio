#pragma once

#include <string>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>
#include <optional>
#include <deque>
#include <LibCore/common.h>

#include "RichName.hpp"
#include "Reflection.hpp"

#include <oishii/reader/binary_reader.hxx>
#include <oishii/v2/writer/binary_writer.hxx>


namespace px {

/*
Before we had filestates, filestatespawnres, importers, exporters and interfaces

Now:
	* File state is composed of folders of nodes--allows for nice interop between languages.
	* Node -> IBinarySerializable (Importer/Exporter)
	* All nodes are reflected like old interfaces.
	* Old spawners replaced by generic factories.
*/

#define PX_TYPE_INFO_EX(gui, cli, internal, icon_pl, icon_sg) \
    constexpr static px::RichName TypeInfo = px::RichName{ gui, internal, cli, {icon_pl, icon_sg } };
#define PX_TYPE_INFO(gui, cli, internal) \
    constexpr static px::RichName TypeInfo = px::RichName{ gui, internal, cli };

#define PX_GET_TID(type) \
	std::string(type::TypeInfo.namespacedId)


// More like IObject
struct IDestructable
{

	//! Our solution to cross-language interop.
	//!
	struct Dynamic
	{
		std::unique_ptr<IDestructable> mOwner;
		void* mBase;
		std::string mType;


		Dynamic(std::unique_ptr<IDestructable> owner, void* base, const std::string& type);
	};
	virtual ~IDestructable() = default;

	virtual std::string getName() const { return "Untitled"; }
	//	// Of type of class.
	//	virtual void* construct() { return nullptr; }

	// Parent
	IDestructable* mParent = nullptr;

	PX_TYPE_INFO_EX("Object", "obj", "IDestructable", "(?)", "(?)");



#define PX_COLLECTION_FOLDER_GETTER(name, type) \
	px::ConcreteCollectionHandle<type> name() { assert(getFolder<type>().has_value()); \
		return px::ConcreteCollectionHandle<type>(getFolder<type>().value()); } \
	const px::ConcreteCollectionHandle<type> name() const { assert(getFolder<type>().has_value()); \
		return px::ConcreteCollectionHandle<type>(const_cast<px::IDestructable::Handle&&>(getFolder<type>().value())); }

	struct Folder
	{
		// Entries
		std::vector<std::unique_ptr<Dynamic>> mNodes; //! All of the indexed files of this folder

		// Selection state
		std::vector<std::size_t> mSelected; //!< Vector of selected indices.
		std::size_t mActiveSelect = 0; //!<< The primary selection. Currently it always exists.

		// Handles
		std::size_t nHandles = 0; //!< The number of open handles for this folder. Don't delete it unless it is zero.
	};

	//! @brief Allow children of a specified type to be added.
	//!
	//! @param[in] type Type of child to accept. Must not be a parent or child of an existing folder.
	//!
	void acceptChild(const std::string& type);

	class Handle
	{
	public:
		//! @brief Get the baseline type of the folder. Nodes must be of this type of children thereof.
		//!
		std::string getType() const;

		//! @brief Get the number of nodes in the folder.
		//!
		std::size_t size() const;

		//! @brief Get the name of a node at a specific index.
		//!
		//! @param[in] idx The index in the node array. Must be less than size().
		//!
		std::string nameAt(u64 idx) const;

		//! @brief Get a node at a specific index casted to a specific type.
		//!
		//! @param[in] idx The index in the node array. Must be less than size().
		//! @param[in] type The type to cast to. Must be registered in the program reflection mesh.
		//!
		void* atAs(u64 idx, const std::string& type);

		//! @brief Get a node at a specific index casted to a specific type.
		//!
		//! @param[in] idx The index in the node array. Must be less than size().
		//! @param[in] type The type to cast to. Must be registered in the program reflection mesh.
		//!
		const void* atAs(u64 idx, const std::string& type) const;

		//! @brief Get a node at a specific index casted to a specific type.
		//!
		//! @tparam type The type to cast to. Must be registered in the program reflection mesh.
		//!
		template<typename T>
		T* at(u64 idx);

		//! @brief Get a node at a specific index casted to a specific type.
		//!
		//! @tparam type The type to cast to. Must be registered in the program reflection mesh.
		//!
		template<typename T>
		const T* at(u64 idx) const;

		Dynamic& atDynamic(u64 idx) { return *folder.mNodes[idx].get(); }

		//! @brief Push a node to the collection.
		//!
		//! @param[in] type The type to cast to. Must be registered in the program reflection mesh.
		//! @param[in] base Pointer to the node of type `type` to avoid dynamic casting.
		//! @param[in] v    Owner of the data being passed in. Pointer to the abstract object interface.
		//!
		void push(const std::string& type, void* base, std::unique_ptr<IDestructable> v);

		//! @brief Push a node to the collection.
		//!
		//! @tparam    T  Type of the node. Must be registered in the program reflection mesh.
		//!
		//! @param[in] in Pointer to the data to move in.
		//!
		template<typename T>
		void push(std::unique_ptr<T> in);

		//! @brief Construct a new node of the specified type and append it to the folder.
		//!
		//! @param[in] type The type of the node to construct. The type must be derived from the folder type or the folder type itself.
		//!
		void add(const std::string& type);

		//! @brief Construct a new node of the folder type and append it. The folder type *must* be constructible.
		//!
		void add();

		//! @brief Return if a node is selected at the specified index.
		//!
		bool isSelected(std::size_t index) const;

		//! @brief Select a node at the specified index.
		//!
		bool select(std::size_t index);

		//! @brief Deselect a node at the specified index.
		//!
		bool deselect(std::size_t index);

		//! @brief Clear the selection. Note: Active selection will not change.
		//!
		//! @return The number of selections prior to clearing.
		//!
		std::size_t clearSelection();

		//! @brief Return the active selection index.
		//!
		std::size_t getActiveSelection() const;

		//! @brief Set the active selection index.
		//!
		//! @param[in] value The new index to use.
		//!
		//! @return The last active selection index.
		//!
		std::size_t setActiveSelection(std::size_t value);

		//! @brief The constructor.
		//!
		//! @parma[in] _type   The type of this handle to wrap. This should match the type of the folder itself.
		//! @param[in] _folder The folder to wrap. Reference counts will be updated.
		//!
		inline Handle(const std::string& _type, Folder& _folder, IDestructable* owner)
			: baseType(_type), folder(_folder), mpOwner(owner)
		{
			++folder.nHandles;
		}

		//! @brief The destructor.
		//!
		inline ~Handle()
		{
			--folder.nHandles;
		}

	private:
		std::string baseType; //!< The base type of this handle. Only children of this type and this type itself may be accessed.
		Folder& folder; //!< The folder being wrapped.
		IDestructable* mpOwner = nullptr; // The current data, to be passed on to children as parent.
	};

	std::optional<Handle> getFolder(const std::string& type, bool fromParent = false, bool fromChild = false) const;
	// const	std::optional<Handle> getFolder(const std::string& type, bool fromParent = false, bool fromChild = false) const;

	template<typename T>
	std::optional<IDestructable::Handle> getFolder() const;
	//	template<typename T>
	//	const	std::optional<IDestructable::Handle> getFolder() const;

	//! @brief Return the number of folders.
	//!
	u32 getNumFolders() const;

	//! @brief Get a folder at the specified index.
	//!
	Handle getFolder(u32 idx);


private:
	std::map<std::string, std::shared_ptr<Folder>> mEntries;
	std::vector<std::string> mLut;
};

template<typename T>
struct ConcreteCollectionHandle : public IDestructable::Handle
{
	ConcreteCollectionHandle(IDestructable::Handle&& c)
		: IDestructable::Handle(c)
	{}

	T& operator[] (std::size_t idx) { return *at<T>(idx); }
	const T& operator[] (std::size_t idx) const { return *at<T>(idx); }
};

using Dynamic = IDestructable::Dynamic;


template<typename T, typename... Args>
Dynamic make_dynamic(Args... args)
{
    auto constructed = std::make_unique<T>(args...);
    void* base = constructed.get();

    return Dynamic(std::move(constructed), base, std::string(T::TypeInfo.namespacedId));
}


struct IBinarySerializer : public IDestructable
{
    PX_TYPE_INFO("Serializable Binary", "binary", "IBinarySerializer");

	virtual std::unique_ptr<IBinarySerializer> clone() = 0;

	virtual bool canRead() { return false; }
	virtual bool canWrite() { return false; }

	virtual void read(Dynamic& state, oishii::BinaryReader& reader) {}
	virtual void write(Dynamic& state, oishii::v2::Writer& writer) {}

	virtual std::string matchFile(const std::string& file, oishii::BinaryReader& reader) { return ""; }
	// TODO: Might merge this with canRead/canWrite
	virtual bool matchForWrite(const std::string& id) { return false; }
};

struct IFactory : public IDestructable
{
    virtual std::unique_ptr<IFactory> clone() = 0;
    virtual Dynamic spawn() = 0;
	std::string_view id;
};

#define PX_FACTORY_NAME(type) __Factory##type
#define PX_FACTORY(type) \
	struct PX_FACTORY_NAME(type) : public px::IFactory { \
		std::unique_ptr<px::IFactory> clone() override { return std::make_unique<PX_FACTORY_NAME(type)>(*this); } \
		px::Dynamic spawn() override { auto obj = px::make_dynamic<type>(); return obj; } \
		PX_FACTORY_NAME(type) () { id = type::TypeInfo.namespacedId; } \
	};

struct PackageInstaller
{
    // DLLs implement their own global instance
    static PackageInstaller* spInstance;

	virtual ~PackageInstaller() = default;

	virtual void registerObject(const RichName& details) = 0;
    virtual void registerFactory(std::unique_ptr<IFactory> factory) = 0;
    virtual void registerSerializer(std::unique_ptr<IBinarySerializer> ser) = 0;
    virtual void registerMirror(const MirrorEntry& entry) = 0;
    virtual void installModule(const std::string& path) = 0;

	virtual Dynamic constructObject(const std::string& type, IDestructable* scene) = 0;

    template<typename D, typename B>
    void registerParent()
    {
        registerMirror({ D::TypeInfo.namespacedId, B::TypeInfo.namespacedId, computeTranslation<D, B>()});
    }
	template<typename D, typename B>
	void registerMember(int slide)
	{
		registerMirror({ D::TypeInfo.namespacedId, B::TypeInfo.namespacedId, slide });
	}
};

// Implementation

inline Dynamic::Dynamic(std::unique_ptr<IDestructable> owner, void* base, const std::string& type)
	: mOwner(std::move(owner)), mBase(base), mType(type)
{}

inline void IDestructable::acceptChild(const std::string& type)
{
	mEntries.emplace(type, std::make_unique<Folder>());
	mLut.emplace_back(type);
}
inline std::optional<IDestructable::Handle> IDestructable::getFolder(const std::string& type, bool fromParent, bool fromChild) const
{
	const auto found = mEntries.find(type);
	if (found != mEntries.end())
		return Handle(type, *(*found).second.get(), (IDestructable*)this);

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
//	template<typename T>
//	inline std::optional<IDestructable::Handle> IDestructable::getFolder()
//	{
//		return getFolder(std::string(T::TypeInfo.namespacedId));
//	}
template<typename T>
inline std::optional<IDestructable::Handle> IDestructable::getFolder() const
{
	return getFolder(std::string(T::TypeInfo.namespacedId));
}

inline u32 IDestructable::getNumFolders() const
{
	return (u32)mEntries.size();
}
inline IDestructable::Handle IDestructable::getFolder(u32 idx)
{
	return { mLut[idx], *mEntries.at(mLut[idx]).get(), this };
}

inline std::string IDestructable::Handle::getType() const { return baseType; }
inline std::size_t IDestructable::Handle::size() const { return folder.mNodes.size(); }

inline std::string IDestructable::Handle::nameAt(u64 idx) const
{
	if (idx >= folder.mNodes.size())
		return "Out of bounds";

	return folder.mNodes[idx]->mOwner->getName();
}
inline void* IDestructable::Handle::atAs(u64 idx, const std::string& type)
{
	if (idx >= folder.mNodes.size())
		return nullptr;
	auto& node = *folder.mNodes[idx].get();
	assert(node.mBase);
	if (node.mType == type)
		return node.mBase;
	std::vector<void*> found;
	ReflectionMesh::getInstance()->findParentOfType(found, node.mBase, node.mType, type);

	assert(found.size() <= 1 && "Ambiguous cast");
	return found.size() == 1 ? found[0] : nullptr;
}
inline const void* IDestructable::Handle::atAs(u64 idx, const std::string& type) const
{
	if (idx >= folder.mNodes.size())
		return nullptr;
	auto& node = *folder.mNodes[idx].get();
	assert(node.mBase);
	if (node.mType == type)
		return node.mBase;
	std::vector<void*> found;
	ReflectionMesh::getInstance()->findParentOfType(found, node.mBase, node.mType, type);

	assert(found.size() <= 1 && "Ambiguous cast");
	return found.size() == 1 ? found[0] : nullptr;
}
inline void IDestructable::Handle::push(const std::string& type, void* base, std::unique_ptr<IDestructable> v)
{
	assert(base);
	assert(v.get());
	v->mParent = mpOwner;
	folder.mNodes.emplace_back(std::unique_ptr<Dynamic>(new Dynamic(std::move(v), base, type)));
}

template<typename T>
inline void IDestructable::Handle::push(std::unique_ptr<T> in)
{
	assert(in);

	void* data = in.get();
	in->mParent = mpOwner;
	push(std::string(T::TypeInfo.namespacedId), data, std::move(in));
}

inline void IDestructable::Handle::add(const std::string& type)
{
	auto constructed = PackageInstaller::spInstance->constructObject(type, nullptr);
	void* base = constructed.mBase;
	// TODO: Set the parent..
	folder.mNodes.emplace_back(std::make_unique<Dynamic>(std::move(constructed.mOwner), base, constructed.mType));
}

inline void IDestructable::Handle::add()
{
	add(baseType);
}

template<typename T>
inline T* IDestructable::Handle::at(u64 idx)
{
	return reinterpret_cast<T*>(atAs(idx, std::string(T::TypeInfo.namespacedId)));
}
template<typename T>
inline const T* IDestructable::Handle::at(u64 idx) const
{
	return reinterpret_cast<const T*>(atAs(idx, std::string(T::TypeInfo.namespacedId)));
}

inline bool IDestructable::Handle::isSelected(std::size_t index) const
{
	const auto& vec = folder.mSelected;
	return std::find(vec.begin(), vec.end(), index) != vec.end();
}

inline bool IDestructable::Handle::select(std::size_t index)
{
	if (isSelected(index)) return true;

	folder.mSelected.push_back(index);
	return false;
}

inline bool IDestructable::Handle::deselect(std::size_t index)
{
	auto it = std::find(folder.mSelected.begin(), folder.mSelected.end(), index);

	if (it == folder.mSelected.end()) return false;
	folder.mSelected.erase(it);
	return true;
}

inline std::size_t IDestructable::Handle::clearSelection()
{
	std::size_t before = folder.mSelected.size();
	folder.mSelected.clear();
	return before;
}

inline std::size_t IDestructable::Handle::getActiveSelection() const
{
	return folder.mActiveSelect;
}

inline std::size_t IDestructable::Handle::setActiveSelection(std::size_t value)
{
	const std::size_t old = folder.mActiveSelect;
	folder.mActiveSelect = value;
	return old;
}


}
