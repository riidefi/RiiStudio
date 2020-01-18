#include <LibCube/JSystem/J3D/Binary/BMD/Sections.hpp>

namespace libcube::jsystem {

void readVTX1(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	if (!enterSection(ctx, 'VTX1'))
		return;

	ScopedSection g(reader, "Vertex Buffers");

	const auto ofsFmt = reader.read<s32>();
	const auto ofsData = reader.readX<s32, 13>();

	reader.seekSet(g.start + ofsFmt);
	for (gx::VertexBufferAttribute type = reader.read<gx::VertexBufferAttribute>();
		type != gx::VertexBufferAttribute::Terminate;
		type = reader.read<gx::VertexBufferAttribute>())
	{
		const auto comp = reader.read<u32>();
		const auto data = reader.read<u32>();
		const auto gen_data = static_cast<gx::VertexBufferType::Generic>(data);
		const auto gen_comp_size = (gen_data == gx::VertexBufferType::Generic::f32) ? 4 :
			(gen_data == gx::VertexBufferType::Generic::u16 || gen_data == gx::VertexBufferType::Generic::s16) ? 2 :
			1;
		const auto shift = reader.read<u8>();
		reader.seek(3);

		assert(0 <= shift && shift <= 31);

		auto estride = 0;
		// FIXME: type punning
		void* buf = nullptr;

		VBufferKind bufkind = VBufferKind::undefined;

		switch (type)
		{
		case gx::VertexBufferAttribute::Position:
			buf = &ctx.mdl.mBufs.pos;
			bufkind = VBufferKind::position;
			ctx.mdl.mBufs.pos.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::Position>(comp)),
				gx::VertexBufferType(gen_data),
				gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0,
				(comp + 2) * gen_comp_size);
			estride = ctx.mdl.mBufs.pos.mQuant.stride;
			break;
		case gx::VertexBufferAttribute::Normal:
			buf = &ctx.mdl.mBufs.norm;
			bufkind = VBufferKind::normal;
			ctx.mdl.mBufs.norm.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::Normal>(comp)),
				gx::VertexBufferType(gen_data),
				gen_data == gx::VertexBufferType::Generic::s8 ? 6 :
				gen_data == gx::VertexBufferType::Generic::s16 ? 14 : 0,
				3 * gen_comp_size
			);
			estride = ctx.mdl.mBufs.norm.mQuant.stride;
			break;
		case gx::VertexBufferAttribute::Color0:
		case gx::VertexBufferAttribute::Color1:
		{
			auto& clr = ctx.mdl.mBufs.color[static_cast<size_t>(type) - static_cast<size_t>(gx::VertexBufferAttribute::Color0)];
			buf = &clr;
			bufkind = VBufferKind::color;
			clr.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::Color>(comp)),
				gx::VertexBufferType(static_cast<gx::VertexBufferType::Color>(data)),
				0,
				(comp + 3)* [](gx::VertexBufferType::Color c) {
					using ct = gx::VertexBufferType::Color;
					switch (c)
					{
					case ct::FORMAT_16B_4444:
					case ct::FORMAT_16B_565:
						return 2;
					case ct::FORMAT_24B_6666:
					case ct::FORMAT_24B_888:
						return 3;
					case ct::FORMAT_32B_8888:
					case ct::FORMAT_32B_888x:
						return 4;
					default:
						throw "Invalid color data type.";
					}
				}(static_cast<gx::VertexBufferType::Color>(data)));
			estride = clr.mQuant.stride;
			break;
		}
		case gx::VertexBufferAttribute::TexCoord0:
		case gx::VertexBufferAttribute::TexCoord1:
		case gx::VertexBufferAttribute::TexCoord2:
		case gx::VertexBufferAttribute::TexCoord3:
		case gx::VertexBufferAttribute::TexCoord4:
		case gx::VertexBufferAttribute::TexCoord5:
		case gx::VertexBufferAttribute::TexCoord6:
		case gx::VertexBufferAttribute::TexCoord7: {
			auto& uv = ctx.mdl.mBufs.uv[static_cast<size_t>(type) - static_cast<size_t>(gx::VertexBufferAttribute::TexCoord0)];
			buf = &uv;
			bufkind = VBufferKind::textureCoordinate;
			uv.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::TextureCoordinate>(comp)),
				gx::VertexBufferType(gen_data),
				gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0,
				(comp + 1)* gen_comp_size
			);
			estride = ctx.mdl.mBufs.pos.mQuant.stride;
			break;
		}
		default:
			assert(false);
		}

		assert(estride);
		assert(bufkind != VBufferKind::undefined);

		const auto getDataIndex = [&](gx::VertexBufferAttribute attr)
		{
			static const constexpr std::array<s8, static_cast<size_t>(gx::VertexBufferAttribute::NormalBinormalTangent) + 1> lut = {
				-1, -1, -1, -1, -1, -1, -1, -1, -1,
				0, // Position
				1, // Normal
				3, 4, // Color
				5, 6, 7, 8, // UV
				9, 10, 11, 12,
				-1, -1, -1, -1, 2
			};

			static_assert(lut[static_cast<size_t>(gx::VertexBufferAttribute::NormalBinormalTangent)] == 2, "NBT");

			const auto attr_i = static_cast<size_t>(attr);

			assert(attr_i < lut.size());

			return attr_i < lut.size() ? lut[attr_i] : -1;
		};

		const auto idx = getDataIndex(type);
		const auto ofs = g.start + ofsData[idx];
		{
			oishii::Jump<oishii::Whence::Set> g_bufdata(reader, ofs);
			s32 size = 0;

			for (int i = idx + 1; i < ofsData.size(); ++i)
			{
				if (ofsData[i])
				{
					size = static_cast<s32>(ofsData[i]) - ofsData[idx];
					break;
				}
			}

			if (!size)
				size = g.size - ofsData[idx];

			assert(size > 0);


			// Desirable to round down
			const auto ensize = (size /*+ estride*/) / estride;
			//	assert(size % estride == 0);
			assert(ensize < u32(u16(-1)) * 3);

			// FIXME: Alignment padding trim

			switch (bufkind)
			{
			case VBufferKind::position: {
				auto pos = reinterpret_cast<decltype(ctx.mdl.mBufs.pos)*>(buf);
					
				pos->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					pos->readBufferEntryGeneric(reader, pos->mData[i]);
				break;
			}
			case VBufferKind::normal: {
				auto nrm = reinterpret_cast<decltype(ctx.mdl.mBufs.norm)*>(buf);

				nrm->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					nrm->readBufferEntryGeneric(reader, nrm->mData[i]);
				break;
			}
			case VBufferKind::color: {
				auto clr = reinterpret_cast<decltype(ctx.mdl.mBufs.color)::value_type*>(buf);

				clr->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					clr->readBufferEntryColor(reader, clr->mData[i]);
				break;
			}
			case VBufferKind::textureCoordinate: {
				auto uv = reinterpret_cast<decltype(ctx.mdl.mBufs.uv)::value_type*>(buf);

				uv->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					uv->readBufferEntryGeneric(reader, uv->mData[i]);
				break;
			}
			}
		}
	}
}

}
