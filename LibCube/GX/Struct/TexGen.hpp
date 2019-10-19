#pragma once

#include "../Enum/Comparison.hpp"
#include <LibRiiEditor/common.hpp>

namespace libcube { namespace gx {

enum class TexGenType
{
	Matrix3x4,
	Matrix2x4,
	Bump0,
	Bump1,
	Bump2,
	Bump3,
	Bump4,
	Bump5,
	Bump6,
	Bump7,
	SRTG
};
enum class TexGenSrc
{
	Position,
	Normal,
	Binormal,
	Tangent,
	UV0,
	UV1,
	UV2,
	UV3,
	UV4,
	UV5,
	UV6,
	UV7,
	BumpUV0,
	BumpUV1,
	BumpUV2,
	BumpUV3,
	BumpUV4,
	BumpUV5,
	BumpUV6,
	Color0,
	Color1
};
enum class PostTexMatrix
{
	Matrix0 = 64,
	Matrix1 = 67,
	Matrix2 = 70,
	Matrix3 = 73,
	Matrix4 = 76,
	Matrix5 = 79,
	Matrix6 = 82,
	Matrix7 = 85,
	Matrix8 = 88,
	Matrix9 = 91,
	Matrix10 = 94,
	Matrix11 = 97,
	Matrix12 = 100,
	Matrix13 = 103,
	Matrix14 = 106,
	Matrix15 = 109,
	Matrix16 = 112,
	Matrix17 = 115,
	Matrix18 = 118,
	Matrix19 = 121,
	Identity = 125
};
enum class TexMatrix
{
	Identity = 60,
	TexMatrix0 = 30,
	TexMatrix1 = 33,
	TexMatrix2 = 36,
	TexMatrix3 = 39,
	TexMatrix4 = 42,
	TexMatrix5 = 45,
	TexMatrix6 = 48,
	TexMatrix7 = 51,
	TexMatrix8 = 54,
	TexMatrix9 = 57
};
struct TexCoordGen // XF TEX/DUALTEX
{
	//	u8			id;
	TexGenType		func;
	TexGenSrc		sourceParam;
	TexMatrix		matrix; // FIXME: for BMD
	
	bool			normalize;
	PostTexMatrix	postMatrix;
};

} } // namespace libcube::gx
