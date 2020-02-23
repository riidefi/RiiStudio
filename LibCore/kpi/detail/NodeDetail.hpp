#pragma once

#include "../Node.hpp"
#include "../Reflection.hpp"

namespace detail {

struct ApplicationPluginsImpl {
	// Requires TypeIdResolvable<T>
	template<typename T>
	struct TFactory final : public IFactory {
		std::unique_ptr<IFactory> clone() const override {
			return std::make_unique<TFactory<T>>(*this);
		}
		std::unique_ptr<IDocumentNode> spawn() override {
			return std::make_unique<TDocumentNode<T>>();
		}
		const char* getId() const override { return typeid(T).name; }
	};
	//! Requires methods:
	//! - `T::canRead(const std::string& file, oishii::BinaryReader& reader) const`
	//! - `T::read(doc_node_t node, oishii::BinaryReader& reader) const`
	template<typename T>
	struct TBinaryDeserializer final : public IBinaryDeserializer, public T {
		std::unique_ptr<IBinaryDeserializer> clone() const override {
			return std::make_unique<TBinaryDeserializer<T>>(*this);
		}
		std::string canRead_(const std::string& file, oishii::BinaryReader& reader) const override {
			return T::canRead(file, reader);
		}
		void read_(doc_node_t node, oishii::BinaryReader& reader) const override {
			T::read(node, reader);
		}
	};
	//! Requires methods:
	//! - `T::canWrite(doc_node_t node) const`
	//! - `T::write(doc_node_t node, oishii::v2::Writer& writer) const`
	template<typename T>
	struct TBinarySerializer final : public IBinarySerializer, public T {
		std::unique_ptr<IBinarySerializer> clone() const override {
			return std::make_unique<TBinarySerializer<T>>(*this);
		}
		std::string canWrite_(doc_node_t node) const override {
			return T::canWrite(node);
		}
		void write_(doc_node_t node, oishii::v2::Writer& writer) const override {
			T::write(node, writer);
		}
	};
	//! Requires: `::write(doc_node_t, oishii::v2::Writer& writer, X*_=nullptr)` where `X` is some child that may be wrapped in a doc_node_t.
	//! No support for inheritance.
	template<typename T>
	struct TSimpleBinarySerializer final : public IBinarySerializer {
		std::unique_ptr<IBinarySerializer> clone() const override {
			return std::make_unique<TSimpleBinarySerializer<T>>(*this);
		}
		bool canWrite_(doc_node_t node) const override {
			return dynamic_cast<T*>(node.get()) != nullptr;
		}
		void write_(doc_node_t node, oishii::v2::Writer& writer) const override {
			write(node, writer, static_cast<T*>(nullptr));
		}
	};
};

}


template<typename T>
void ApplicationPlugins::addType() { mFactories[typeid(T).name] = std::make_unique<detail::ApplicationPluginsImpl::TFactory<T>>(); }

template<typename T>
void ApplicationPlugins::addSerializer() { mWriters.push_back(std::make_unique<detail::ApplicationPluginsImpl::TBinarySerializer<T>>(); }

template<typename T>
void ApplicationPlugins::addSimpleSerializer() { mWriters.push_back(std::make_unique<detail::ApplicationPluginsImpl::TSimpleBinarySerializer<T>>(); }

template<typename T>
void ApplicationPlugins::addDeserializer() { mReaders.push_back(std::make_unique<detail::ApplicationPluginsImpl::TBinaryDeserializer<T>>(); }
