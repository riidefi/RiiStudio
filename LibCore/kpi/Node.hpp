/*

Application:
    - Type hierarchy
    - Factories
    - Serializers
Document:
    - Flat pool of elements (boxed)
    - Hierarchy

Each element standalone.
Wrapper:
    What type is the element?
    -> Spawners / factory
    Some link to hierarchy element
    Selection state
*/

#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <optional>

#include <LibCore/common.h>

#include <ThirdParty/immer/vector.hpp>
#include <ThirdParty/immer/box.hpp>
#include <ThirdParty/immer/map.hpp>

namespace oishii {
	struct BinaryReader;
	namespace v2 { struct Writer; }
}

struct SelectionState {
    std::vector<int> selectedChildren;
    int activeSelectChild;
};

struct IDocumentNode;
using doc_node_t = immer::box<IDocumentNode>;


class History {
public:
	void commit() {
		assert(root_history.size());
		root_history.erase(root_history.begin() + history_cursor, root_history.end());
		root_history.push_back(root_history.back());
		++history_cursor;
	}
	void undo() {
		assert(history_cursor > 1 && root_history.size() > 1 && history_cursor < root_history.size());
		--history_cursor;
	}
	void redo() {
		++history_cursor;
	}
	doc_node_t currentDocument() {
		return root_history[history_cursor];
	}
	void changeCurrentDocument(doc_node_t x) {
		root_history[history_cursor] = x;
	}

private:
	// At the roots, we don't need persistence
	// We don't ever expose history to anyone -- only the current document
	std::vector<doc_node_t> root_history;
	std::size_t history_cursor = 0;
};

// we know use typeid for type
class IDocumentNode {
public:
    virtual ~IDocumentNode() = 0;

	using FolderData = immer::vector<doc_node_t>;

	std::optional<FolderData> getFolder(const std::string& type, bool fromParent = false, bool fromChild = false) {
		const auto* found = children.find(type);
		if (found)
			return *found;

		auto info = px::ReflectionMesh::getInstance()->lookupInfo(type);

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
	std::optional<FolderData> getFolder() {
		return getFolder(typeid(T).name);
	}

	SelectionState select;
	std::optional<doc_node_t> parent;
	immer::map<std::string, FolderData> children;
	std::vector<std::string> folderLut;
};

// Might be huge, say a vertex array -- and likely never changed!
template<typename T>
struct TDocumentNode final : public IDocumentNode, public T {
	TDocumentNode() = default;
	// Potential overrides: getParent, getChild, getNextSibling, etc
};



/*
Application:
    - Type hierarchy
    - Factories: Construct type
    - Serializers: Write/Read constructed type
*/

// Part of the application state itself. Not part of the persistent document.
class ApplicationPlugins {
	friend class ApplicationPluginsImpl;
public:
	// virtual void registerObject(const RichName& details) = 0;

	//! @brief Add a type to the internal registry for future construction and manipulation.
	//!
	//! @tparam T Any default constructible type.
	//!
	//! @details Example:
	//!				struct SomeType
	//!				{
	//!					SomeType() = default;
	//!					int value = 0;
	//!				};
	//!
	//!				ApplicationPlugins::getInstance()->addType<SomeType>();
	//!
    template<typename T>
	void addType();

	//! @brief Add a binary serializer (writer) to the internal registry.
	//!
	//! @tparam T Any default constructible type T with member functions:
	//!				- `bool T::canWrite(doc_node_t node) const` (return if we can write)
	//!				- `T::write(doc_node_t node, oishii::v2::Writer& writer) const` (write the file)
	//!
	//! @details Example:
	//!				struct SomeWriter
	//!				{
	//!					bool canWrite(doc_node_t node) const
	//!					{
	//!						// If `node` is not of our type `SomeType` from before or a child of it, we cannot write it.
	//!						const SomeType* data = dynamic_cast<const SomeType*>(node.get());
	//!						if (data == nullptr) {
	//!							return false;
	//!						}
	//!
	//!						// For example, we might not be able to express negative numbers in this format.
	//!						if (data->value < 0) {
	//!							return false;
	//!						}
	//!						
	//!						return true;
	//!					}
	//!					void write(doc_node_t node, oishii::v2::Writer& writer) const
	//!					{
	//!						// We will not be here unless our last check passed.
	//!						const SomeType* data = dynamic_cast<const SomeType*>(node.get());
	//!						assert(data != nullptr);
	//!
	//!						// Write our file magic / identifier.
	//!						writer.write<u8>('S');
	//!						writer.write<u8>('O');
	//!						writer.write<u8>('M');
	//!						writer.write<u8>('E');
	//!						// Write our data
	//!						writer.write<s32>(data->value);
	//!					}
	//!				};
	//!
	//!				ApplicationPlugins::getInstance()->addSerializer<SomeWriter>();
	//!
	template<typename T>
	void addSerializer();

	//! @brief Add a binary serializer (writer) to the internal registry with a simplified API.
	//!
	//! @tparam T Any default constructible type T where the following function exists:
	//!				- `::write(doc_node_t, oishii::v2::Writer& writer, X*_=nullptr)` where `X` is some child that may be wrapped in a doc_node_t.
	//!
	//! @details Example:
	//!				// `dummy` is necessary to distinguish this `write` function from other `write` functions.
	//!				// It will always be nullptr.
	//!				void write(doc_node_t node, oishii::v2::Writer& writer, SomeType* dummy=nullptr) const
	//!				{
	//!					// We will not be here unless node is a child of SomeType or SomeType itself.
	//!					const SomeType* data = dynamic_cast<const SomeType*>(node.get());
	//!					assert(data != nullptr);
	//!
	//!					// Write our file magic / identifier.
	//!					writer.write<u8>('S');
	//!					writer.write<u8>('O');
	//!					writer.write<u8>('M');
	//!					writer.write<u8>('E');
	//!					// Write our data
	//!					writer.write<s32>(data->value);
	//!				}
	//!
	//!				ApplicationPlugins::getInstance()->addSimpleSerializer<SomeType>();
	//!
	template<typename T>
	void addSimpleSerializer();

	//! @brief Add a binary deserializer (reader) to the internal registry.
	//!
	//! @tparam T Any default constructible type T with member functions:
	//!				- `std::string T::canRead(const std::string& file, oishii::BinaryReader& reader) const` (return the type of the file identified or empty)
	//!				- `T::read(doc_node_t node, oishii::BinaryReader& reader) const` (read the file)
	//!
	//! @details Example:
	//!				struct SomeReader
	//!				{
	//!					std::string canRead(const std::string& file, oishii::BinaryReader& reader) const
	//!					{
	//!						// Our file is eight bytes long.
	//!						if (reader.endpos() != 8) {
	//!							return "";
	//!						}
	//!
	//!						// Our file uses the file magic / identifier 'SOME'.
	//!						if (reader.peek<u8>(0) != 'S' ||
	//!							reader.peek<u8>(1) != 'O' ||
	//!							reader.peek<u8>(2) != 'M' ||
	//!							reader.peek<u8>(3) != 'E') {
	//!							return "";
	//!						}
	//!
	//!						// Use the name of SomeType to identify it later.
	//!						return typeid(SomeType).name;
	//!					}
	//!					void read(doc_node_t node, oishii::BinaryReader& writer) const
	//!					{
	//!						// Ensure that the constructed document node is derived from or of type SomeType.
	//!						const SomeType* data = dynamic_cast<const SomeType*>(node.get());
	//!						assert(data != nullptr);
	//!
	//!						// We already checked our magic in our `canRead` function. We can ignore it.
	//!						reader.seek(4);
	//!
	//!						// Read our data
	//!						data->value = reader.read<s32>();
	//!					}
	//!				};
	//!
	//!				ApplicationPlugins::getInstance()->addDeserializer<SomeReader>();
	//!
	template<typename T>
	void addDeserializer();

    virtual void registerMirror(const px::MirrorEntry& entry) = 0;
    virtual void installModule(const std::string& path) = 0;

	virtual doc_node_t constructObject(const std::string& type, std::optional<doc_node_t> parent = {}) = 0;

    ApplicationPlugins* getInstance();

private:
	struct IFactory {
		virtual ~IFactory() = default;
		virtual std::unique_ptr<IFactory> clone() const = 0;
		virtual std::unique_ptr<IDocumentNode> spawn() = 0;
		virtual const char* getId() const = 0;
	};
	//! A reader
	struct IBinaryDeserializer {
		virtual ~IBinaryDeserializer() = default;
		virtual std::unique_ptr<IBinaryDeserializer> clone() const = 0;
		virtual std::string canRead_(const std::string& file, oishii::BinaryReader& reader) const = 0;
		virtual void read_(doc_node_t node, oishii::BinaryReader& reader) const = 0;
	};
	//! A writer
	struct IBinarySerializer {
		virtual ~IBinarySerializer() = default;
		virtual std::unique_ptr<IBinarySerializer> clone() const = 0;
		virtual bool canWrite_(doc_node_t node) const = 0;
		virtual void write_(doc_node_t node, oishii::v2::Writer& writer) const = 0;
	};

    std::map<std::string, std::unique_ptr<IFactory>> mFactories;
	std::vector<std::unique_ptr<IBinaryDeserializer>> mReaders;
	std::vector<std::unique_ptr<IBinarySerializer>> mWriters;
};

#include "detail/NodeDetail.hpp"
