/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "BMD.hpp"
#include <LibCube/JSystem/J3D/Model.hpp>
#include <map>
#include <type_traits>
#include <algorithm>
#include <tuple>

#include <oishii/writer/node.hxx>
#include <oishii/writer/linker.hxx>

#include "SceneGraph.hpp"
#include "Sections.hpp"

namespace libcube::jsystem {

void BMDImporter::lex(BMDOutputContext& ctx, u32 sec_count) noexcept
{
	ctx.mSections.clear();
	for (u32 i = 0; i < sec_count; ++i)
	{
		const u32 secType = ctx.reader.read<u32>();
		const u32 secSize = ctx.reader.read<u32>();

		{
			oishii::JumpOut g(ctx.reader, secSize - 8);
			switch (secType)
			{
			case 'INF1':
			case 'VTX1':
			case 'EVP1':
			case 'DRW1':
			case 'JNT1':
			case 'SHP1':
			case 'MAT3':
			case 'MDL3':
			case 'TEX1':
				ctx.mSections[secType] = { ctx.reader.tell(), secSize };
				break;
			default:
				ctx.reader.warnAt("Unexpected section type.", ctx.reader.tell() - 8, ctx.reader.tell() - 4);
				break;
			}
		}
	}
}
void BMDImporter::readBMD(oishii::BinaryReader& reader, BMDOutputContext& ctx)
{
	reader.setEndian(true);
	reader.expectMagic<'J3D2'>();


	u32 bmdVer = reader.read<u32>();
	if (bmdVer != 'bmd3' && bmdVer != 'bdl4')
	{
		reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
		error = true;
		return;
	}

	// TODO: Validate file size.
	const auto fileSize = reader.read<u32>();
	const auto sec_count = reader.read<u32>();

	// Skip SVR3 header
	reader.seek<oishii::Whence::Current>(16);

	// Skim through sections
	lex(ctx, sec_count);

	// Read vertex data
	readVTX1(ctx);

	// Read joints
	readJNT1(ctx);

	// Read envelopes and draw matrices
	readEVP1DRW1(ctx);

	// Read shapes
	readSHP1(ctx);

	// Read TEX1

	// Read materials
	readMAT3(ctx);

	// Read INF1
	readINF1(ctx);

	// Read MDL3
}
bool BMDImporter::tryRead(oishii::BinaryReader& reader, pl::FileState& state)
{
	//oishii::BinaryReader::ScopedRegion g(reader, "J3D Binary Model Data (.bmd)");
	auto& j3dc = static_cast<J3DCollection&>(state);
	readBMD(reader, BMDOutputContext{ j3dc.mModel, reader });
	return !error;
}


template<typename T>
struct LinkNode final : public oishii::Node, T
{
	LinkNode(bool b=false)
		: T(), Node(T::getNameId())
	{
		transferOwnershipToLinker(b);
	}
	template<typename S>
	LinkNode(S& arg)
		: T(arg), Node(T::getNameId())
	{
		transferOwnershipToLinker(true);
	}
	LinkNode* asLeaf()
	{
		getLinkingRestriction().options |= oishii::LinkingRestriction::Leaf;
		return this;
	}
	eResult write(oishii::Writer& writer) const noexcept
	{
		T::write(writer);
		return eResult::Success;
	}

	eResult gatherChildren(std::vector<const oishii::Node*>& out) const noexcept override
	{
		for (auto ptr : T::gatherChildren())
			out.push_back(ptr.get());

		return eResult::Success;
	}
	const oishii::Node& getSelf() const override
	{
		return *this;
	}
};
template<typename T>
union linker_ptr
{
	const T* get() const { return data; }
	T* get() { return data; }

	T* operator->() { return get(); }
	const T* operator->() const { return get(); }


	linker_ptr(T* ptr)
		: data(ptr)
	{}
	linker_ptr(const linker_ptr& other)
	{
		this->data = other.data;
	}

private:
	T* data;
};

template<typename T>
auto make_tempnode = []()
{
	// Deleted by linker
	return linker_ptr<LinkNode<T>>(new LinkNode<T>(true));
};

struct INF1Node
{
	static const char* getNameId() { return "INF1 InFormation"; }
	virtual const oishii::Node& getSelf() const = 0;

	void write(oishii::Writer& writer) const
	{
		if (!mdl) return;

		writer.write<u32, oishii::EndianSelect::Big>('INF1');
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("VTX1"/*getSelf(), oishii::Hook::EndOfChildren*/) });

		writer.write<u16>(static_cast<u16>(mdl->info.mScalingRule) & 0xf);
		writer.write<u16>(-1);
		// Packet count
		writer.write<u32>(-1); // TODO
		// Vertex position count
		writer.write<u32>(mdl->mBufs.pos.mData.size());

		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("SceneGraph")
			});
	}


	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{
		return { SceneGraph::getLinkerNode(*mdl, true) };
	}

	J3DModel* mdl = nullptr;
};

struct VTX1Node
{
	static const char* getNameId() { return "VTX1"; }
	virtual const oishii::Node& getSelf() const = 0;

	int computeNumOfs() const
	{
		int numOfs = 0;
		if (mdl)
		{
			if (!mdl->mBufs.pos.mData.empty()) ++numOfs;
			if (!mdl->mBufs.norm.mData.empty()) ++numOfs;
			for (int i = 0; i < 2; ++i)
				if (!mdl->mBufs.color[i].mData.empty()) ++numOfs;
			for (int i = 0; i < 8; ++i)
				if (!mdl->mBufs.uv[i].mData.empty()) ++numOfs;
		}
		return numOfs;
	}

	void write(oishii::Writer& writer) const
	{
		if (!mdl) return;

		writer.write<u32, oishii::EndianSelect::Big>('VTX1');
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook(getSelf(), oishii::Hook::EndOfChildren) });

		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("Format") });

		const auto numOfs = computeNumOfs();
		int i = 0;
		auto writeBufLink = [&]()
		{
			writer.writeLink<s32>(oishii::Link{
				oishii::Hook(getSelf()),
				oishii::Hook("Buf" + std::to_string(i++)) });
		};
		auto writeOptBufLink = [&](const auto& b) {
			if (b.mData.empty())
				writer.write<s32>(0);
			else
				writeBufLink();
		};

		writeOptBufLink(mdl->mBufs.pos);
		writeOptBufLink(mdl->mBufs.norm);
		writer.write<s32>(0); // NBT

		for (const auto& c : mdl->mBufs.color)
			writeOptBufLink(c);

		for (const auto& u : mdl->mBufs.uv)
			writeOptBufLink(u);
	}

	struct FormatDecl
	{
		static const char* getNameId() { return "Format"; }
		virtual const oishii::Node& getSelf() const = 0;

		struct Entry
		{
			gx::VertexBufferAttribute attrib;
			u32 cnt = 1;
			u32 data = 0;
			u8 shift = 0;

			void write(oishii::Writer& writer)
			{
				writer.write<u32>(static_cast<u32>(attrib));
				writer.write<u32>(static_cast<u32>(cnt));
				writer.write<u32>(static_cast<u32>(data));
				writer.write<u8>(static_cast<u8>(shift));
				for (int i = 0; i < 3; ++i)
					writer.write<u8>(-1);
			}
		};

		void write(oishii::Writer& writer) const
		{
			// Positions
			if (!mdl->mBufs.pos.mData.empty())
			{
				const auto& q = mdl->mBufs.pos.mQuant;
				Entry{
					gx::VertexBufferAttribute::Position,
					static_cast<u32>(q.comp.position),
					static_cast<u32>(q.type.generic),
					q.divisor
				}.write(writer);
			}
			// Normals
			if (!mdl->mBufs.norm.mData.empty())
			{
				const auto& q = mdl->mBufs.norm.mQuant;
				Entry{
					gx::VertexBufferAttribute::Normal,
					static_cast<u32>(q.comp.normal),
					static_cast<u32>(q.type.generic),
					q.divisor
				}.write(writer);
			}
			// Colors
			{
				int i = 0;
				for (const auto& buf : mdl->mBufs.color)
				{
					if (!buf.mData.empty())
					{
						const auto& q = buf.mQuant;
						Entry{
							gx::VertexBufferAttribute((int)gx::VertexBufferAttribute::Color0 + i),
							static_cast<u32>(q.comp.color),
							static_cast<u32>(q.type.color),
							q.divisor
						}.write(writer);
					}
					++i;
				}
			}
			// UVs
			{
				int i = 0;
				for (const auto& buf : mdl->mBufs.uv)
				{
					if (!buf.mData.empty())
					{
						const auto& q = buf.mQuant;
						Entry{
							gx::VertexBufferAttribute((int)gx::VertexBufferAttribute::TexCoord0 + i),
							static_cast<u32>(q.comp.texcoord),
							static_cast<u32>(q.type.generic),
							q.divisor
						}.write(writer);
					}
					++i;
				}
			}
			Entry{ gx::VertexBufferAttribute::Terminate }.write(writer);
		}
		std::vector<linker_ptr<oishii::Node>> gatherChildren() const
		{
			return {};
		}

		J3DModel* mdl;
	};

	template<typename T>
	struct VertexAttribBuf : public oishii::Node
	{
		VertexAttribBuf(const J3DModel& m, const std::string& id, const T& data, bool linkerOwned = true)
			: Node(id), mdl(m), mData(data)
		{
			transferOwnershipToLinker(linkerOwned);
			getLinkingRestriction().options |= oishii::LinkingRestriction::Leaf;
		}

		eResult write(oishii::Writer& writer) const noexcept
		{
			mData.writeData(writer);
			return eResult::Success;
		}

		const J3DModel& mdl;
		const T& mData;
	};

	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{
		std::vector<linker_ptr<oishii::Node>> tmp;

		if (!mdl)
			return tmp;

		auto fmt = new LinkNode<FormatDecl>();
		fmt->mdl = mdl;
		tmp.emplace_back(fmt);

		int i = 0;

		auto push_buf = [&](auto& buf) {
			auto b = new VertexAttribBuf(*mdl, "Buf" + std::to_string(i++), buf);
			b->getLinkingRestriction().alignment = 32;
			tmp.emplace_back(b);
		};

		// Positions
		if (!mdl->mBufs.pos.mData.empty())
		{
			push_buf(mdl->mBufs.pos);
			// tmp.emplace_back(new VertexAttribBuf(*mdl, "Buf" + std::to_string(i++), mdl->mBufs.pos));
		}
		// Normals
		if (!mdl->mBufs.norm.mData.empty())
			push_buf(mdl->mBufs.norm);
		// Colors
		for (auto& c : mdl->mBufs.color)
			if (!c.mData.empty())
				push_buf(c);
		// UV
		for (auto& c : mdl->mBufs.uv)
			if (!c.mData.empty())
				push_buf(c);


		return tmp;
	}

	J3DModel* mdl = nullptr;
};

struct EVP1Node
{
	static const char* getNameId() { return "EVP1 EnVeloPe"; }
	virtual const oishii::Node& getSelf() const = 0;

	void write(oishii::Writer& writer) const
	{
		writer.write<u32, oishii::EndianSelect::Big>('EVP1');
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("VTX1"/*getSelf(), oishii::Hook::EndOfChildren*/) });

		writer.write<u16>(envelopesToWrite.size());
		writer.write<u16>(-1);
		// ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixSizeTable")
		});
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixIndexTable")
		});
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixWeightTable")
		});
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixInvBindTable")
		});
	}
	struct SimpleEvpNode
	{
		virtual const oishii::Node& getSelf() const = 0;
		std::vector<linker_ptr<oishii::Node>> gatherChildren() const { return{}; }

		SimpleEvpNode(const std::vector<DrawMatrix>& from, const std::vector<int>& toWrite, const std::vector<float>& weightPool)
			: mFrom(from), mToWrite(toWrite), mWeightPool(weightPool)
		{}
		const std::vector<DrawMatrix>& mFrom;
		const std::vector<int>& mToWrite;
		const std::vector<float>& mWeightPool;
	};
	struct MatrixSizeTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixSizeTable"; }
		MatrixSizeTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			for (int i : mToWrite)
				writer.write<u8>(mFrom[i].mWeights.size());
		}
	};
	struct MatrixIndexTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixIndexTable"; }
		MatrixIndexTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			for (int i : mToWrite)
				writer.write<u8>(i);
		}
	};
	struct MatrixWeightTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixWeightTable"; }
		MatrixWeightTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			for (f32 f: mWeightPool)
				writer.write<f32>(f);
		}
	};
	struct MatrixInvBindTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixInvBindTable"; }
		MatrixInvBindTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			// TODO
		}
	};
	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{

		return {
			// Matrix size table
			(new LinkNode<MatrixSizeTable>(*this))->asLeaf(),
			// Matrix index table
			(new LinkNode<MatrixIndexTable>(*this))->asLeaf(),
			// matrix weight table
			(new LinkNode<MatrixWeightTable>(*this))->asLeaf(),
			// matrix inv bind table
			(new LinkNode<MatrixInvBindTable>(*this))->asLeaf()
		};
	}

	EVP1Node(J3DModel& jmdl)
		: mdl(jmdl)
	{
		for (int i = 0; i < mdl.mDrawMatrices.size(); ++i)
		{
			if (mdl.mDrawMatrices[i].mWeights.size() <= 1)
			{
				assert(mdl.mDrawMatrices[i].mWeights[0].weight == 1.0f);
			}
			else
			{
				envelopesToWrite.push_back(i);
				for (const auto& it : mdl.mDrawMatrices[i].mWeights)
				{
					if (std::find(weightPool.begin(), weightPool.end(), it.weight) == weightPool.end())
						weightPool.push_back(it.weight);
				}
			}
		}
	}
	std::vector<f32> weightPool;
	std::vector<int> envelopesToWrite;
	J3DModel& mdl;
};
struct BMDFile
{
	static const char* getNameId() { return "JSystem Binary Model Data"; }

	virtual const oishii::Node& getSelf() const = 0;

	void write(oishii::Writer& writer) const
	{
		// Hack
		writer.write<u32, oishii::EndianSelect::Big>('J3D2');
		writer.write<u32, oishii::EndianSelect::Big>(bBDL ? 'bdl4' : 'bmd3');

		// Filesize
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook(getSelf(), oishii::Hook::EndOfChildren) });

		// 8 sections
		writer.write<u32>(bBDL ? 9 : 8);

		// SubVeRsion
		writer.write<u32, oishii::EndianSelect::Big>('SVR3');
		for (int i = 0; i < 3; ++i)
			writer.write<u32>(-1);
	}
	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{
		auto* inf1 = new LinkNode<INF1Node>();
		inf1->getLinkingRestriction().alignment = 32;
		inf1->mdl = &mCollection->mModel;

		auto* vtx1 = new LinkNode<VTX1Node>();
		vtx1->getLinkingRestriction().alignment = 32;
		vtx1->mdl = &mCollection->mModel;

		// evp1
		auto* evp1 = new LinkNode<EVP1Node>(mCollection->mModel);
		evp1->getLinkingRestriction().alignment = 32;
		// drw1
		// jnt1
		// shp1
		// mat3
		// tex1

		return { inf1, vtx1, evp1 };
	}

	J3DCollection* mCollection;
	bool bBDL = true;
	bool bMimic = true;
};

void exportBMD(oishii::Writer& writer, J3DCollection& collection)
{
	oishii::Linker linker;

	LinkNode<BMDFile> bmd(false);
	bmd.bBDL = collection.bdl;
	bmd.bMimic = true;
	bmd.mCollection = &collection;

	linker.gather(bmd, "");
	linker.write(writer);
}

bool BMDExporter::write(oishii::Writer& writer, pl::FileState& state)
{
	exportBMD(writer, static_cast<J3DCollection&>(state));

	return true;
}

} // namespace libcube::jsystem
