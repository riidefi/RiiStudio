#include <ThirdParty/glfw/glfw3.h>

#include "GXProgram.hpp"

namespace libcube {

using namespace gx;

std::string GXProgram::generateMaterialSource(const gx::ChannelControl& chan, int i)
{
    switch (chan.Material)
    {
        case ColorSource::Vertex: return "a_Color" + std::to_string(i);
        case ColorSource::Register: return "u_ColorMatReg[" + std::to_string(i) + "]";
    }
}

std::string GXProgram::generateAmbientSource(const gx::ChannelControl& chan, int i)
{
    switch (chan.Ambient)
    {
        case ColorSource::Vertex: return "a_Color" + std::to_string(i);
        case ColorSource::Register: return "u_ColorAmbReg[" + std::to_string(i) + "]";
    }
}

std::string GXProgram::generateLightDiffFn(const gx::ChannelControl& chan, const std::string& lightName)
{
    const std::string NdotL = "dot(t_Normal, t_LightDeltaDir)";

    switch (chan.diffuseFn)
    {
        case DiffuseFunction::None: return "1.0";
        case DiffuseFunction::Sign: return NdotL;
        case DiffuseFunction::Clamp: return "max(" + NdotL + ", 0.0f)";
    }
}
std::string GXProgram::generateLightAttnFn(const gx::ChannelControl& chan, const std::string& lightName)
{
    const std::string cosAttn = "ApplyCubic(" + lightName + ".CosAtten.xyz, dot(t_LightDeltaDir, " + lightName + ".Direction.xyz))";

    switch (chan.attenuationFn)
    {
        case AttenuationFunction::None: return "1.0";
        case AttenuationFunction::Spotlight: return "max(" + cosAttn + " / max(dot(" + lightName + ".DistAtten.xyz, vec3(1.0, t_LightDeltaDist2, t_LightDeltaDist)), 0.0), 0.0)";
        case AttenuationFunction::Specular: return "1.0"; // TODO(jtspierre): Specular
    }
}
std::string GXProgram::generateColorChannel(const gx::ChannelControl& chan, const std::string& outputName, int i)
{
    const std::string matSource = generateMaterialSource(chan, i);
    const std::string ambSource = generateAmbientSource(chan, i);

    std::string generateLightAccum = "";

    if (chan.enabled)
    {
        generateLightAccum = "t_LightAccum = " + ambSource+ ";\n";

        if (chan.lightMask != LightID::None)
            assert(mMaterial.hasLightsBlock);

		for (int j = 0; j < 8; j++)
		{
			if (!(u32(chan.lightMask) & (1 << j)))
				continue;

			const std::string lightName = "u_LightParams[" + std::to_string(j) + "]";
			generateLightAccum +=
				"    t_LightDelta = " + lightName + ".Position.xyz - v_Position.xyz;\n"
				"    t_LightDeltaDist2 = dot(t_LightDelta, t_LightDelta);\n"
				"    t_LightDeltaDist = sqrt(t_LightDeltaDist2);\n"
				"    t_LightDeltaDir = t_LightDelta / t_LightDeltaDist;\n"
				"    t_LightAccum += " + generateLightDiffFn(chan, lightName) + " * " + generateLightAttnFn(chan, lightName) + " * " + lightName + ".Color;\n";
		}
    }
    else
    {
        // Without lighting, everything is full-bright.
        generateLightAccum += "    t_LightAccum = vec4(1.0);\n";
    }
    return generateLightAccum + "    " + outputName + " = " + matSource + " * clamp(t_LightAccum, 0.0, 1.0);\n"; // .trim();
}

std::string GXProgram::generateLightChannel(const LightingChannelControl& lightChannel, const std::string& outputName, int i)
{
    if (lightChannel.colorChannel == lightChannel.alphaChannel)
    {
		// TODO
        return "    " + generateColorChannel(lightChannel.colorChannel, outputName, i);
    }
    else
    {
		return
			generateColorChannel(lightChannel.colorChannel, "t_ColorChanTemp", i) + "\n" +
			outputName + ".rgb = t_ColorChanTemp.rgb;\n" +
			generateColorChannel(lightChannel.alphaChannel, "t_ColorChanTemp", i) + "\n" +
			outputName + ".a = t_ColorChanTemp.a;\n";
	}
}

std::string GXProgram::generateLightChannels()
{
    std::string out;


	return "";
    //for(const auto& chan : mMaterial.lightChannels)
    //    out += generateLightChannel(chan, "v_Color" + std::to_string(i), i) + "\n";

    return out;
}

// Matrix
std::string GXProgram::generateMulPntMatrixStatic(gx::PostTexMatrix pnt, const std::string& src)
{
	if (pnt == gx::PostTexMatrix::Identity
		|| (int)pnt == (int)gx::TexMatrix::Identity) {
		return src + ".xyz";
	}
	else if (pnt >= gx::PostTexMatrix::Matrix0) {
		const int pnMtxIdx = (((int)pnt - (int)gx::PostTexMatrix::Matrix0)) / 3;
		return "(u_PosMtx[" + std::to_string(pnMtxIdx) + "] * " + src + ")";
	}
	else if ((int)pnt >= (int)gx::TexMatrix::TexMatrix0) {
		const int texMtxIdx = (((int)pnt - (int)gx::TexMatrix::TexMatrix0)) / 3;
		return "(u_TexMtx[" + std::to_string(texMtxIdx) + "] * " + src + ")";
	}
	else {
		throw "whoops";
	}

	return "";
}
// Output is a vec3, src is a vec4.
std::string GXProgram::generateMulPntMatrixDynamic(const std::string& attrStr, const std::string& src)
{
	return "(GetPosTexMatrix(" + attrStr + ") * " + src + ")";
}
std::string GXProgram::generateTexMtxIdxAttr(int index)
{
	switch (index)
	{
	case 0: return "a_TexMtx0123Idx.x";
	case 1: return "a_TexMtx0123Idx.y";
	case 2: return "a_TexMtx0123Idx.z";
	case 3: return "a_TexMtx0123Idx.w";
	case 4: return "a_TexMtx4567Idx.x";
	case 5: return "a_TexMtx4567Idx.y";
	case 6: return "a_TexMtx4567Idx.z";
	case 7: return "a_TexMtx4567Idx.w";
	default:
		throw "whoops";
	}

	return "";
}
//----------------------------------
// TexGen

// Output is a vec4.
//#if 0
std::string GXProgram::generateTexGenSource(gx::TexGenSrc src)
{
	switch (src) {
	case gx::TexGenSrc::Position:  return "vec4(a_Position, 1.0)";
	case gx::TexGenSrc::Normal:    return "vec4(a_Normal, 1.0)";
	case gx::TexGenSrc::Color0:    return "v_Color0";
	case gx::TexGenSrc::Color1:    return "v_Color1";
	case gx::TexGenSrc::UV0:      return "vec4(a_Tex0, 1.0, 1.0)";
	case gx::TexGenSrc::UV1:      return "vec4(a_Tex1, 1.0, 1.0)";
	case gx::TexGenSrc::UV2:      return "vec4(a_Tex2, 1.0, 1.0)";
	case gx::TexGenSrc::UV3:      return "vec4(a_Tex3, 1.0, 1.0)";
	case gx::TexGenSrc::UV4:      return "vec4(a_Tex4, 1.0, 1.0)";
	case gx::TexGenSrc::UV5:      return "vec4(a_Tex5, 1.0, 1.0)";
	case gx::TexGenSrc::UV6:      return "vec4(a_Tex6, 1.0, 1.0)";
	case gx::TexGenSrc::UV7:      return "vec4(a_Tex7, 1.0, 1.0)";
	case gx::TexGenSrc::BumpUV0: return "vec4(v_TexCoord0, 1.0)";
	case gx::TexGenSrc::BumpUV1: return "vec4(v_TexCoord1, 1.0)";
	case gx::TexGenSrc::BumpUV2: return "vec4(v_TexCoord2, 1.0)";
	case gx::TexGenSrc::BumpUV3: return "vec4(v_TexCoord3, 1.0)";
	case gx::TexGenSrc::BumpUV4: return "vec4(v_TexCoord4, 1.0)";
	case gx::TexGenSrc::BumpUV5: return "vec4(v_TexCoord5, 1.0)";
	case gx::TexGenSrc::BumpUV6: return "vec4(v_TexCoord6, 1.0)";
	default:
		throw "whoops";
	}
	return "";
}
// Output is a vec3, src is a vec4.
std::string GXProgram::generatePostTexGenMatrixMult(const gx::TexCoordGen& texCoordGen, int id, const std::string& src)
{
	if (texCoordGen.postMatrix == gx::PostTexMatrix::Identity) {
		return src + ".xyz";
	}
	else if (texCoordGen.postMatrix >= gx::PostTexMatrix::Matrix0) {
		const u32 texMtxIdx = ((u32)texCoordGen.postMatrix - (u32)gx::PostTexMatrix::Matrix0) / 3;
		assert(texMtxIdx < 10);
		return "(u_PostTexMtx[" + std::to_string(texMtxIdx) + std::string("] * ") + src + ")";
	}
	else {
		throw "whoops";
		return "";
	}
}
// Output is a vec3, src is a vec3.
vec3_string GXProgram::generateTexGenMatrixMult(const gx::TexCoordGen& texCoordGen, int id, const vec3_string& src)
{
	// TODO: Will ID ever be different from index?

	// Dynamic TexMtxIdx is off by default.
	if (mMaterial.useTexMtxIdx[id]) {
		const std::string attrStr = generateTexMtxIdxAttr(id);
		return generateMulPntMatrixDynamic(attrStr, src);
	}
	else {
		return generateMulPntMatrixStatic((gx::PostTexMatrix)texCoordGen.matrix, src);
	}
}

// Output is a vec3, src is a vec4.
vec3_string GXProgram::generateTexGenType(const gx::TexCoordGen& texCoordGen, int id, const vec4_string& src) {
	switch (texCoordGen.func) {
	case gx::TexGenType::SRTG:
		return "vec3(" + src + ".xy, 1.0)";
	case gx::TexGenType::Matrix2x4:
		return "vec3(" + generateTexGenMatrixMult(texCoordGen, id, src) + ".xy, 1.0)";
	case gx::TexGenType::Matrix3x4:
		return generateTexGenMatrixMult(texCoordGen, id, src);
	default:
		throw "whoops";
	}
}

// Output is a vec3.
std::string GXProgram::generateTexGenNrm(const gx::TexCoordGen& texCoordGen, int id) {
	const auto src = generateTexGenSource(texCoordGen.sourceParam);
	const auto type = generateTexGenType(texCoordGen, id, src);
	if (texCoordGen.normalize)
		return "normalize(" + type + ")";
	else
		return type;
}
// Output is a vec3.
std::string GXProgram::generateTexGenPost(const gx::TexCoordGen& texCoordGen, int id)
{
	const auto src = generateTexGenNrm(texCoordGen, id);

	if (texCoordGen.postMatrix == gx::PostTexMatrix::Identity)
		return src;
	else
		return generatePostTexGenMatrixMult(texCoordGen, id,  "vec4(" + src + ", 1.0)");
}

std::string GXProgram::generateTexGen(const gx::TexCoordGen& texCoordGen, int id)
{
	return "v_TexCoord" + std::to_string(/*texCoordGen.*/id) + " = " + generateTexGenPost(texCoordGen, id) + ";\n";
}

std::string GXProgram::generateTexGens() {
	std::string out;
	const auto& tgs = mMaterial.mat.getMaterialData().texGens;
	for (int i = 0; i < tgs.size(); ++i)
		out += generateTexGen(tgs[i], i);
	return out;
}

std::string GXProgram::generateTexCoordGetters()
{
	std::string out;
	for (int i = 0; i < mMaterial.mat.getMaterialData().texGens.size(); ++i)
	{
		const std::string is = std::to_string(i);
		out += "vec2 ReadTexCoord" + is + "() { return v_TexCoord" + is + ".xy / v_TexCoord" + is + ".z; }\n";
	}
	return out;
}

// IndTex
std::string GXProgram::generateIndTexStageScaleN(gx::IndirectTextureScalePair::Selection scale)
{
    switch (scale) {
    case gx::IndirectTextureScalePair::Selection::x_1: return "1.0";
    case gx::IndirectTextureScalePair::Selection::x_2: return "1.0/2.0";
    case gx::IndirectTextureScalePair::Selection::x_4: return "1.0/4.0";
    case gx::IndirectTextureScalePair::Selection::x_8: return "1.0/8.0";
    case gx::IndirectTextureScalePair::Selection::x_16: return "1.0/16.0";
    case gx::IndirectTextureScalePair::Selection::x_32: return "1.0/32.0";
    case gx::IndirectTextureScalePair::Selection::x_64: return "1.0/64.0";
    case gx::IndirectTextureScalePair::Selection::x_128: return "1.0/128.0";
    case gx::IndirectTextureScalePair::Selection::x_256: return "1.0/256.0";
    }
}

std::string GXProgram::generateIndTexStageScale(const gx::TevStage::IndirectStage& stage,
	const gx::IndirectTextureScalePair& scale, const gx::IndOrder& mIndOrder)
{
	const std::string baseCoord = "ReadTexCoord" + std::to_string(mIndOrder.refCoord) + "()";

	if (scale.U == gx::IndirectTextureScalePair::Selection::x_1 && scale.V == gx::IndirectTextureScalePair::Selection::x_1)
        return baseCoord;
    else
        return baseCoord + " * vec2(" + generateIndTexStageScaleN(scale.U) + ", " + generateIndTexStageScaleN(scale.V) + ")";
}
	
std::string GXProgram::generateTextureSample(u32 index, const std::string& coord)
{
	const auto idx_str = std::to_string(index);
    return "texture(u_Texture[" + idx_str + "], " +
		coord + ", TextureLODBias(" + idx_str + "))";
}

std::string GXProgram::generateIndTexStage(u32 indTexStageIndex)
{
    const auto& stage = mMaterial.mat.getMaterialData().shader.mStages[indTexStageIndex].indirectStage;
    return "vec3 t_IndTexCoord" + std::to_string(indTexStageIndex) + " = 255.0 * " +
		generateTextureSample(mMaterial.mat.getMaterialData().shader.mIndirectOrders[indTexStageIndex].refMap,
			generateIndTexStageScale(stage, mMaterial.mat.getMaterialData().mIndScales[indTexStageIndex], mMaterial.mat.getMaterialData().shader.mIndirectOrders[indTexStageIndex])) + ".abg;\n";
}

std::string GXProgram::generateIndTexStages()
{
	std::string out;

	for (int i = 0; i < mMaterial.mat.getMaterialData().shader.mStages.size(); ++i)
	{
		if (mMaterial.mat.getMaterialData().shader.mIndirectOrders[i].refCoord >= mMaterial.mat.getMaterialData().texGens.size())
			continue;
		out += generateIndTexStage(i);
	}
	return out;
}

// TEV
std::string GXProgram::generateKonstColorSel(gx::TevKColorSel konstColor)
{
	switch (konstColor) {
	case gx::TevKColorSel::const_8_8:    return "vec3(8.0/8.0)";
	case gx::TevKColorSel::const_7_8:  return "vec3(7.0/8.0)";
	case gx::TevKColorSel::const_6_8:  return "vec3(6.0/8.0)";
	case gx::TevKColorSel::const_5_8:  return "vec3(5.0/8.0)";
	case gx::TevKColorSel::const_4_8:  return "vec3(4.0/8.0)";
	case gx::TevKColorSel::const_3_8:  return "vec3(3.0/8.0)";
	case gx::TevKColorSel::const_2_8:  return "vec3(2.0/8.0)";
	case gx::TevKColorSel::const_1_8:  return "vec3(1.0/8.0)";
	case gx::TevKColorSel::k0:   return "s_kColor0.rgb";
	case gx::TevKColorSel::k0_r: return "s_kColor0.rrr";
	case gx::TevKColorSel::k0_g: return "s_kColor0.ggg";
	case gx::TevKColorSel::k0_b: return "s_kColor0.bbb";
	case gx::TevKColorSel::k0_a: return "s_kColor0.aaa";
	case gx::TevKColorSel::k1:   return "s_kColor1.rgb";
	case gx::TevKColorSel::k1_r: return "s_kColor1.rrr";
	case gx::TevKColorSel::k1_g: return "s_kColor1.ggg";
	case gx::TevKColorSel::k1_b: return "s_kColor1.bbb";
	case gx::TevKColorSel::k1_a: return "s_kColor1.aaa";
	case gx::TevKColorSel::k2:   return "s_kColor2.rgb";
	case gx::TevKColorSel::k2_r: return "s_kColor2.rrr";
	case gx::TevKColorSel::k2_g: return "s_kColor2.ggg";
	case gx::TevKColorSel::k2_b: return "s_kColor2.bbb";
	case gx::TevKColorSel::k2_a: return "s_kColor2.aaa";
	case gx::TevKColorSel::k3:   return "s_kColor3.rgb";
	case gx::TevKColorSel::k3_r: return "s_kColor3.rrr";
	case gx::TevKColorSel::k3_g: return "s_kColor3.ggg";
	case gx::TevKColorSel::k3_b: return "s_kColor3.bbb";
	case gx::TevKColorSel::k3_a: return "s_kColor3.aaa";
	}
}

std::string GXProgram::generateKonstAlphaSel(gx::TevKAlphaSel konstAlpha)
{
	switch (konstAlpha) {
	case gx::TevKAlphaSel::const_8_8:    return "(8.0/8.0)";
	case gx::TevKAlphaSel::const_7_8:  return "(7.0/8.0)";
	case gx::TevKAlphaSel::const_6_8:  return "(6.0/8.0)";
	case gx::TevKAlphaSel::const_5_8:  return "(5.0/8.0)";
	case gx::TevKAlphaSel::const_4_8:  return "(4.0/8.0)";
	case gx::TevKAlphaSel::const_3_8:  return "(3.0/8.0)";
	case gx::TevKAlphaSel::const_2_8:  return "(2.0/8.0)";
	case gx::TevKAlphaSel::const_1_8:  return "(1.0/8.0)";
	case gx::TevKAlphaSel::k0_r: return "s_kColor0.r";
	case gx::TevKAlphaSel::k0_g: return "s_kColor0.g";
	case gx::TevKAlphaSel::k0_b: return "s_kColor0.b";
	case gx::TevKAlphaSel::k0_a: return "s_kColor0.a";
	case gx::TevKAlphaSel::k1_r: return "s_kColor1.r";
	case gx::TevKAlphaSel::k1_g: return "s_kColor1.g";
	case gx::TevKAlphaSel::k1_b: return "s_kColor1.b";
	case gx::TevKAlphaSel::k1_a: return "s_kColor1.a";
	case gx::TevKAlphaSel::k2_r: return "s_kColor2.r";
	case gx::TevKAlphaSel::k2_g: return "s_kColor2.g";
	case gx::TevKAlphaSel::k2_b: return "s_kColor2.b";
	case gx::TevKAlphaSel::k2_a: return "s_kColor2.a";
	case gx::TevKAlphaSel::k3_r: return "s_kColor3.r";
	case gx::TevKAlphaSel::k3_g: return "s_kColor3.g";
	case gx::TevKAlphaSel::k3_b: return "s_kColor3.b";
	case gx::TevKAlphaSel::k3_a: return "s_kColor3.a";
	}
}

std::string GXProgram::generateRas(const gx::TevStage& stage) {
	switch (stage.rasOrder) {
	case gx::ColorSelChanApi::color0a0:   return "v_Color0";
	case gx::ColorSelChanApi::color1a1:   return "v_Color1";
	case gx::ColorSelChanApi::color0: return "vec4(0, 0, 0, 0)";
	default:
		throw "";
	}
}

std::string GXProgram::generateTexAccess(const gx::TevStage& stage) {
	// Skyward Sword is amazing sometimes. I hope you"re happy...
	// assert(stage.texMap !== GX.TexMapID.TEXMAP_NULL);
	if (stage.texMap == -1)
		return "vec4(1.0, 1.0, 1.0, 1.0)";

	//	// If we disable textures, then return sampled white.
	//	if (this.hacks !== null && this.hacks.disableTextures)
	//	    return "vec4(1.0, 1.0, 1.0, 1.0)";

	return generateTextureSample(stage.texMap, "t_TexCoord");
}
std::string GXProgram::generateComponentSwizzle(const gx::SwapTableEntry* swapTable, gx::ColorComponent channel)
{
	const char* suffixes[] = { "r", "g", "b", "a" };
	if (swapTable)
		channel = swapTable->lookup(channel);
	return suffixes[(u8)channel];
}

std::string GXProgram::generateColorSwizzle(const gx::SwapTableEntry* swapTable,
	gx::TevColorArg colorIn)
{
	const auto swapR = generateComponentSwizzle(swapTable, gx::ColorComponent::r);
	const auto swapG = generateComponentSwizzle(swapTable, gx::ColorComponent::g);
	const auto swapB = generateComponentSwizzle(swapTable, gx::ColorComponent::b);
	const auto swapA = generateComponentSwizzle(swapTable, gx::ColorComponent::a);

	switch (colorIn) {
	case gx::TevColorArg::texc:
	case gx::TevColorArg::rasc:
		return swapR + swapG + swapB;
	case gx::TevColorArg::texa:
	case gx::TevColorArg::rasa:
		return swapA + swapA + swapA;
	default:
		throw "whoops";
	}
}

std::string GXProgram::generateColorIn(const gx::TevStage& stage, gx::TevColorArg colorIn)
{
	switch (colorIn) {
	case gx::TevColorArg::cprev: return "t_ColorPrev.rgb";
	case gx::TevColorArg::aprev: return "t_ColorPrev.aaa";
	case gx::TevColorArg::c0:    return "t_Color0.rgb";
	case gx::TevColorArg::a0:    return "t_Color0.aaa";
	case gx::TevColorArg::c1:    return "t_Color1.rgb";
	case gx::TevColorArg::a1:    return "t_Color1.aaa";
	case gx::TevColorArg::c2:    return "t_Color2.rgb";
	case gx::TevColorArg::a2:    return "t_Color2.aaa";
	case gx::TevColorArg::texc:
		return generateTexAccess(stage) + "." + generateColorSwizzle(&mMaterial.mat.getMaterialData().shader.mSwapTable[stage.texMapSwap], colorIn);
	case gx::TevColorArg::texa:
		return generateTexAccess(stage) + "." + generateColorSwizzle(&mMaterial.mat.getMaterialData().shader.mSwapTable[stage.texMapSwap], colorIn);
	case gx::TevColorArg::rasc:
		return "TevSaturate(" + generateRas(stage) + "." + generateColorSwizzle(&mMaterial.mat.getMaterialData().shader.mSwapTable[stage.rasSwap], colorIn) + ")";
	case gx::TevColorArg::rasa:
		return "TevSaturate(" + generateRas(stage) + "." + generateColorSwizzle(&mMaterial.mat.getMaterialData().shader.mSwapTable[stage.rasSwap], colorIn) + ")";
	case gx::TevColorArg::one:   return "vec3(1)";
	case gx::TevColorArg::half:  return "vec3(1.0/2.0)";
	case gx::TevColorArg::konst:
		return generateKonstColorSel(stage.colorStage.constantSelection);
	case gx::TevColorArg::zero:  return "vec3(0)";
	}
}


std::string GXProgram::generateAlphaIn(const gx::TevStage& stage, gx::TevAlphaArg alphaIn)
{
	switch (alphaIn) {
	case gx::TevAlphaArg::aprev: return "t_ColorPrev.a";
	case gx::TevAlphaArg::a0:    return "t_Color0.a";
	case gx::TevAlphaArg::a1:    return "t_Color1.a";
	case gx::TevAlphaArg::a2:    return "t_Color2.a";
	case gx::TevAlphaArg::texa:
		return generateTexAccess(stage) + "." + generateComponentSwizzle(&mMaterial.mat.getMaterialData().shader.mSwapTable[stage.texMapSwap], gx::ColorComponent::a);
	case gx::TevAlphaArg::rasa:
		return "TevSaturate(" + generateRas(stage) + "." + generateComponentSwizzle(&mMaterial.mat.getMaterialData().shader.mSwapTable[stage.rasSwap], gx::ColorComponent::a) + ")";
	case gx::TevAlphaArg::konst:
		return generateKonstAlphaSel(stage.alphaStage.constantSelection);
	case gx::TevAlphaArg::zero:  return "0.0";
	}
}


std::string GXProgram::generateTevInputs(const gx::TevStage& stage)
{
	return R"(
    t_TevA = TevOverflow(vec4()" + generateColorIn(stage, stage.colorStage.a) + ", " + generateAlphaIn(stage, stage.alphaStage.a) + R"());
    t_TevB = TevOverflow(vec4()" + generateColorIn(stage, stage.colorStage.b) + ", " + generateAlphaIn(stage, stage.alphaStage.b) + R"());
    t_TevC = TevOverflow(vec4()" + generateColorIn(stage, stage.colorStage.c) + ", " + generateAlphaIn(stage, stage.alphaStage.c) + R"());
    t_TevD = vec4()" + generateColorIn(stage, stage.colorStage.d) + ", " + generateAlphaIn(stage, stage.alphaStage.d) + ");\n"
; //.trim();
}

std::string GXProgram::generateTevRegister(gx::TevReg regId)
{
	switch (regId) {
	case gx::TevReg::prev: return "t_ColorPrev";
	case gx::TevReg::reg0: return "t_Color0";
	case gx::TevReg::reg1: return "t_Color1";
	case gx::TevReg::reg2: return "t_Color2";
	}
}

std::string GXProgram::generateTevOpBiasScaleClamp(const std::string& value, gx::TevBias bias, gx::TevScale scale)
{
	auto v = value;

	if (bias == gx::TevBias::add_half)
		v = "TevBias(" + v + ", 0.5)";
	else if (bias == gx::TevBias::sub_half)
		v = "TevBias(" + v + ", -0.5)";

	if (scale == gx::TevScale::scale_2)
		v = "(" + v + ") * 2.0";
	else if (scale == gx::TevScale::scale_4)
		v = "(" + v + ") * 4.0";
	else if (scale == gx::TevScale::divide_2)
		v = "(" + v + ") * 0.5";

	return v;
}

std::string GXProgram::generateTevOp(gx::TevColorOp op,
	gx::TevBias bias, gx::TevScale scale, const std::string& a, const std::string& b,
	const std::string& c, const std::string& d, const std::string& zero)
{
	switch (op) {
	case gx::TevColorOp::add:
	case gx::TevColorOp::subtract:
	{
		const auto neg = (op == gx::TevColorOp::subtract) ? "-" : "";
		const auto v = neg + std::string("mix(") + a + ", " + b + ", " + c + ") + " + d;
		return generateTevOpBiasScaleClamp(v, bias, scale);
	}
	case gx::TevColorOp::comp_r8_gt:     return "((t_TevA.r >  t_TevB.r) ? " + c + " : " + zero + ") + " + d;
	case gx::TevColorOp::comp_r8_eq:     return "((t_TevA.r == t_TevB.r) ? " + c + " : " + zero + ") + " + d;
	case gx::TevColorOp::comp_gr16_gt:   return "((TevPack16(t_TevA.rg) >  TevPack16(t_TevB.rg)) ? " + c + " : " + zero + ") + " + d;
	case gx::TevColorOp::comp_gr16_eq:   return "((TevPack16(t_TevA.rg) == TevPack16(t_TevB.rg)) ? " + c + " : " + zero + ") + " + d;
	case gx::TevColorOp::comp_bgr24_gt:  return "((TevPack24(t_TevA.rgb) >  TevPack24(t_TevB.rgb)) ? "+ c + " : " + zero + ") + " + d;
	case gx::TevColorOp::comp_bgr24_eq:  return "((TevPack24(t_TevA.rgb) == TevPack24(t_TevB.rgb)) ? "+ c + " : " + zero + ") + " + d;
	case gx::TevColorOp::comp_rgb8_gt:   return "(TevPerCompGT(${a}, ${b}) * ${c}) + ${d}";
	case gx::TevColorOp::comp_rgb8_eq:   return "(TevPerCompEQ(${a}, ${b}) * ${c}) + ${d}";
	default:
		throw "";
	}

	return "";
}

std::string GXProgram::generateTevOpValue(gx::TevColorOp op, gx::TevBias bias, gx::TevScale scale, bool clamp,
	const std::string& a, const std::string& b, const std::string& c, const std::string& d, const std::string& zero)
{
	const auto expr = generateTevOp(op, bias, scale, a, b, c, d, zero);

	if (clamp)
		return "TevSaturate(" + expr + ")";
	else
		return expr;
}

std::string GXProgram::generateColorOp(const gx::TevStage& stage)
{
	const auto a = "t_TevA.rgb", b = "t_TevB.rgb", c = "t_TevC.rgb", d = "t_TevD.rgb", zero = "vec3(0)";
	const auto value = generateTevOpValue(stage.colorStage.formula, stage.colorStage.bias, stage.colorStage.scale, stage.colorStage.clamp, a, b, c, d, zero);
	return generateTevRegister(stage.colorStage.out) + ".rgb = " + value + ";\n";
}

std::string GXProgram::generateAlphaOp(const gx::TevStage& stage)
{
	const auto a = "t_TevA.a", b = "t_TevB.a", c = "t_TevC.a", d = "t_TevD.a", zero = "0.0";
	const auto value = generateTevOpValue(static_cast<gx::TevColorOp>(stage.alphaStage.formula), stage.alphaStage.bias, stage.alphaStage.scale, stage.alphaStage.clamp, a, b, c, d, zero);
	return generateTevRegister(stage.alphaStage.out) + ".a = " + value + ";\n";
}

std::string GXProgram::generateTevTexCoordWrapN(const std::string& texCoord, gx::IndTexWrap wrap)
{
	switch (wrap) {
	case gx::IndTexWrap::off:  return texCoord;
	case gx::IndTexWrap::_0:   return "0.0";
	case gx::IndTexWrap::_256: return "mod(" + texCoord + ", 256.0)";
	case gx::IndTexWrap::_128: return "mod(" + texCoord + ", 128.0)";
	case gx::IndTexWrap::_64:  return "mod(" + texCoord + ", 64.0)";
	case gx::IndTexWrap::_32:  return "mod(" + texCoord + ", 32.0)";
	case gx::IndTexWrap::_16:  return "mod(" + texCoord + ", 16.0)";
	}
}


std::string GXProgram::generateTevTexCoordWrap(const gx::TevStage& stage)
{
	const int lastTexGenId = mMaterial.mat.getMaterialData().texGens.size() - 1;
	int texGenId = stage.texCoord;

	if (texGenId >= lastTexGenId)
		texGenId = lastTexGenId;
	if (texGenId < 0)
		return "vec2(0.0, 0.0)";

	const auto baseCoord = "ReadTexCoord" + std::to_string(texGenId) + "()";
	if (stage.indirectStage.wrapU == gx::IndTexWrap::off && stage.indirectStage.wrapV == gx::IndTexWrap::off)
		return baseCoord;
	else
		return "vec2(" + generateTevTexCoordWrapN(baseCoord + ".x", stage.indirectStage.wrapU) + ", " +
			generateTevTexCoordWrapN(baseCoord + ".y", stage.indirectStage.wrapV) + ")";
}

std::string GXProgram::generateTevTexCoordIndTexCoordBias(const gx::TevStage& stage)
{
	const std::string bias = (stage.indirectStage.format == gx::IndTexFormat::_8bit) ? "-128.0" : "1.0";

	switch (stage.indirectStage.bias) {
	case gx::IndTexBiasSel::none: return "";
	case gx::IndTexBiasSel::s:    return " + vec3(" + bias + ", 0.0, 0.0)";
	case gx::IndTexBiasSel::st:   return " + vec3(" + bias + ", " + bias + ", 0.0)";
	case gx::IndTexBiasSel::su:   return " + vec3(" + bias + ", 0.0, " + bias + ")";
	case gx::IndTexBiasSel::t:    return " + vec3(0.0, " + bias + ", 0.0)";
	case gx::IndTexBiasSel::tu:   return " + vec3(0.0, " + bias + ", " + bias + ")";
	case gx::IndTexBiasSel::u:    return " + vec3(0.0, 0.0, " + bias + ")";
	case gx::IndTexBiasSel::stu:  return " + vec3(" + bias + ")";
	}
}

std::string GXProgram::generateTevTexCoordIndTexCoord(const gx::TevStage& stage) {
	const auto baseCoord = "(t_IndTexCoord" + std::to_string(stage.indirectStage.indStageSel) + ")";
	switch (stage.indirectStage.format) {
	case gx::IndTexFormat::_8bit: return baseCoord;
	default: throw "";
	}
}

std::string GXProgram::generateTevTexCoordIndirectMtx(const gx::TevStage& stage)
{
	const auto indTevCoord = "(" + generateTevTexCoordIndTexCoord(stage) + generateTevTexCoordIndTexCoordBias(stage) + ")";

	switch (stage.indirectStage.matrix) {
	case gx::IndTexMtxID::_0:  return "(u_IndTexMtx[0] * vec4(" + indTevCoord + ", 0.0))";
	case gx::IndTexMtxID::_1:  return "(u_IndTexMtx[1] * vec4(" + indTevCoord + ", 0.0))";
	case gx::IndTexMtxID::_2:  return "(u_IndTexMtx[2] * vec4(" + indTevCoord + ", 0.0))";
	default:
		printf("Unimplemented indTexMatrix mode: %u\n", (u32)stage.indirectStage.matrix);
		return indTevCoord + ".xy";
	}
}

std::string GXProgram::generateTevTexCoordIndirectTranslation(const gx::TevStage& stage)
{
	return "(" + generateTevTexCoordIndirectMtx(stage) + " * TextureInvScale(" + std::to_string(stage.texMap) + "))";
}

std::string GXProgram::generateTevTexCoordIndirect(const gx::TevStage& stage)
{
	const auto baseCoord = generateTevTexCoordWrap(stage);

	if (stage.indirectStage.matrix != gx::IndTexMtxID::off &&
		stage.indirectStage.indStageSel < mMaterial.mat.getMaterialData().shader.mStages.size())
		return baseCoord + " + " + generateTevTexCoordIndirectTranslation(stage);
	else
		return baseCoord;
}

std::string GXProgram::generateTevTexCoord(const gx::TevStage& stage)
{
	if (stage.texCoord == -1)
		return "";

	const auto finalCoord = generateTevTexCoordIndirect(stage);
	if (stage.indirectStage.addPrev) {
		return "t_TexCoord += " + finalCoord + ";\n";
	}
	else {
		return "t_TexCoord = " + finalCoord + ";\n";
	}
}

std::string GXProgram::generateTevStage(u32 tevStageIndex)
{
	const auto& stage = mMaterial.mat.getMaterialData().shader.mStages[tevStageIndex];

	return 
		"// TEV Stage " + std::to_string(tevStageIndex) + "\n" +
		generateTevTexCoord(stage) +
		generateTevInputs(stage) +
		generateColorOp(stage) +
		generateAlphaOp(stage);
}

std::string GXProgram::generateTevStages() {
	std::string out;
	for (int i = 0; i < mMaterial.mat.getMaterialData().shader.mStages.size(); ++i)
		out += generateTevStage(i);
	return out;
}

std::string GXProgram::generateTevStagesLastMinuteFixup() {
	const auto& tevStages = mMaterial.mat.getMaterialData().shader.mStages;

	const auto& lastTevStage = tevStages[tevStages.size() - 1];
	const auto colorReg = generateTevRegister(lastTevStage.colorStage.out);
	const auto alphaReg = generateTevRegister(lastTevStage.alphaStage.out);
	if (colorReg == alphaReg) {
		return "vec4 t_TevOutput = " + colorReg + ";\n";
	}
	else {
		return "vec4 t_TevOutput = vec4(" + colorReg + ".rgb, " + alphaReg + ".a);\n";
	}
}

std::string GXProgram::generateAlphaTestCompare(gx::Comparison compare, u8 reference)
{
	const auto ref = std::to_string(static_cast<f32>(reference));
	switch (compare) {
	case gx::Comparison::NEVER:   return "false";
	case gx::Comparison::LESS:    return "t_PixelOut.a <  " + ref;
	case gx::Comparison::EQUAL:   return "t_PixelOut.a == " + ref;
	case gx::Comparison::LEQUAL:  return "t_PixelOut.a <= " + ref;
	case gx::Comparison::GREATER: return "t_PixelOut.a >  " + ref;
	case gx::Comparison::NEQUAL:  return "t_PixelOut.a != " + ref;
	case gx::Comparison::GEQUAL:  return "t_PixelOut.a >= " + ref;
	case gx::Comparison::ALWAYS:  return "true";
	}
}

std::string GXProgram::generateAlphaTestOp(gx::AlphaOp op)
{
	switch (op) {
	case gx::AlphaOp::_and:  return "t_AlphaTestA && t_AlphaTestB";
	case gx::AlphaOp::_or:   return "t_AlphaTestA || t_AlphaTestB";
	case gx::AlphaOp::_xor:  return "t_AlphaTestA != t_AlphaTestB";
	case gx::AlphaOp::_xnor: return "t_AlphaTestA == t_AlphaTestB";
	}
}
std::string GXProgram::generateAlphaTest()
{
	const auto alphaTest = mMaterial.mat.getMaterialData().alphaCompare;
	return
		"	bool t_AlphaTestA = " + generateAlphaTestCompare(alphaTest.compLeft, alphaTest.refLeft) + ";\n"
		"	bool t_AlphaTestB = " + generateAlphaTestCompare(alphaTest.compRight, alphaTest.refRight) + ";\n"
		"	if (!(" + generateAlphaTestOp(alphaTest.op) + "))\n"
		"		//discard; \n";
}
std::string GXProgram::generateFogZCoord() {
	return "";
}
std::string GXProgram::generateFogBase() {
	return "";
}

std::string GXProgram::generateFogAdj(const std::string& base)
{
	return "";
}

std::string GXProgram::generateFogFunc(const std::string& base)
{
	return "";
}

std::string GXProgram::generateFog()
{
	return "";
}
std::string GXProgram::generateAttributeStorageType(u32 glFormat, u32 count)
{
	assert(glFormat == GL_FLOAT);
	if (glFormat != GL_FLOAT) throw "whoops";

	switch (count)
	{
	case 1:  return "float";
	case 2:  return "vec2";
	case 3:  return "vec3";
	case 4:  return "vec4";
	default: throw "whoops";
	}
	return "";
}

std::string GXProgram::generateVertAttributeDefs()
{
	std::string out;
	int i = 0;
	for (const auto& attr : vtxAttributeGenDefs)
	{
		// if (attr.format != GL_FLOAT) continue;
		out += "layout(location = " + std::to_string(i) + ") in " +
			generateAttributeStorageType(attr.format, attr.size) + " a_" + attr.name + ";\n";
		++i;
	}
	return out;
}
std::string GXProgram::generateMulPos()
{
	// Default to using pnmtxidx.
	const auto src = "vec4(a_Position, 1.0)";
	if (mMaterial.usePnMtxIdx)
		return generateMulPntMatrixDynamic("uint(a_PnMtxIdx)", src);
	else
		return generateMulPntMatrixStatic(gx::PostTexMatrix::Matrix0, src);
}

std::string GXProgram::generateMulNrm()
{
	// Default to using pnmtxidx.
	const auto src = "vec4(a_Normal, 0.0)";
	// TODO(jstpierre): Move to a normal matrix calculated on the CPU
	if (mMaterial.usePnMtxIdx)
		return generateMulPntMatrixDynamic("uint(a_PnMtxIdx)", src);
	else
		return generateMulPntMatrixStatic(gx::PostTexMatrix::Matrix0, src);
}

std::pair<std::string, std::string> GXProgram::generateShaders()
{
	const auto bindingsDefinition = generateBindingsDefinition(mMaterial.hasPostTexMtxBlock, mMaterial.hasLightsBlock);
		
	const auto both = R"(#version 440
// )" + mMaterial.mat.getName() + R"(
precision mediump float;
)" +
bindingsDefinition;
const std::string varying_vert = 
R"(out vec3 v_Position;
out vec4 v_Color0;
out vec4 v_Color1;
out vec3 v_TexCoord0;
out vec3 v_TexCoord1;
out vec3 v_TexCoord2;
out vec3 v_TexCoord3;
out vec3 v_TexCoord4;
out vec3 v_TexCoord5;
out vec3 v_TexCoord6;
out vec3 v_TexCoord7;
)";
const std::string varying_frag =
R"(in vec3 v_Position;
in vec4 v_Color0;
in vec4 v_Color1;
in vec3 v_TexCoord0;
in vec3 v_TexCoord1;
in vec3 v_TexCoord2;
in vec3 v_TexCoord3;
in vec3 v_TexCoord4;
in vec3 v_TexCoord5;
in vec3 v_TexCoord6;
in vec3 v_TexCoord7;
)";

	const auto vert = std::string(both) + varying_vert + generateVertAttributeDefs() +
		"mat4x3 GetPosTexMatrix(uint t_MtxIdx) {\n"
		"    if (t_MtxIdx == " + std::to_string((int)gx::TexMatrix::Identity) + "u)\n"
		"        return mat4x3(1.0);\n"
		"    else if (t_MtxIdx >= " + std::to_string((int)gx::TexMatrix::TexMatrix0) + "u)\n"
		"        return u_TexMtx[(t_MtxIdx - " + std::to_string((int)gx::TexMatrix::TexMatrix0) + "u) / 3u];\n"
		"    else\n"
		"        return u_PosMtx[t_MtxIdx / 3u];\n"
		"}\n" +
		R"(
float ApplyAttenuation(vec3 t_Coeff, float t_Value) {
    return dot(t_Coeff, vec3(1.0, t_Value, t_Value*t_Value));
}
)" +
"void main() {\n"
"    vec3 t_Position = " + generateMulPos() + ";\n"
"    v_Position = t_Position;\n"
"    vec3 t_Normal = " + generateMulNrm() + ";\n"
"    vec4 t_LightAccum;\n"
"    vec3 t_LightDelta, t_LightDeltaDir;\n"
"    float t_LightDeltaDist2, t_LightDeltaDist, t_Attenuation;\n"
"    vec4 t_ColorChanTemp;\n"
"    v_Color0 = a_Color0;"
+ generateLightChannels() + generateTexGens() +
"gl_Position = (u_Projection * vec4(t_Position, 1.0));\n"
"}\n";

	const auto frag = both + varying_frag + generateTexCoordGetters() + R"(
float TextureLODBias(int index) { return u_SceneTextureLODBias + u_TextureParams[index].w; }
vec2 TextureInvScale(int index) { return 1.0 / u_TextureParams[index].xy; }
vec2 TextureScale(int index) { return u_TextureParams[index].xy; }
vec3 TevBias(vec3 a, float b) { return a + vec3(b); }
float TevBias(float a, float b) { return a + b; }
vec3 TevSaturate(vec3 a) { return clamp(a, vec3(0), vec3(1)); }
float TevSaturate(float a) { return clamp(a, 0.0, 1.0); }
float TevOverflow(float a) { return float(int(a * 255.0) & 255) / 255.0; }
vec4 TevOverflow(vec4 a) { return vec4(TevOverflow(a.r), TevOverflow(a.g), TevOverflow(a.b), TevOverflow(a.a)); }
float TevPack16(vec2 a) { return dot(a, vec2(1.0, 256.0)); }
float TevPack24(vec3 a) { return dot(a, vec3(1.0, 256.0, 256.0 * 256.0)); }
float TevPerCompGT(float a, float b) { return float(a >  b); }
float TevPerCompEQ(float a, float b) { return float(a == b); }
vec3 TevPerCompGT(vec3 a, vec3 b) { return vec3(greaterThan(a, b)); }
vec3 TevPerCompEQ(vec3 a, vec3 b) { return vec3(greaterThan(a, b)); }
void main() {
    vec4 s_kColor0   = u_KonstColor[0];
    vec4 s_kColor1   = u_KonstColor[1];
    vec4 s_kColor2   = u_KonstColor[2];
    vec4 s_kColor3   = u_KonstColor[3];
    vec4 t_ColorPrev = u_Color[0];
    vec4 t_Color0    = u_Color[1];
    vec4 t_Color1    = u_Color[2];
    vec4 t_Color2    = u_Color[3];
)" +
		generateIndTexStages() + R"(
    vec2 t_TexCoord = vec2(0.0, 0.0);
    vec4 t_TevA, t_TevB, t_TevC, t_TevD;)" +
		generateTevStages() +
		generateTevStagesLastMinuteFixup() +
	"    vec4 t_PixelOut = TevOverflow(t_TevOutput);\n" +
		generateAlphaTest() +
		generateFog() +
		"    gl_FragColor = vec4(v_Color0.xyz, 0.5);"
		// "    gl_FragColor = t_PixelOut;\n"
		"}\n";
	return { vert, frag };
}
u32 translateCullMode(gx::CullMode cullMode)
{
    switch (cullMode) {
	case gx::CullMode::All:
        return GL_FRONT_AND_BACK;
	case gx::CullMode::Front:
        return GL_FRONT;
	case gx::CullMode::Back:
        return GL_BACK;
	case gx::CullMode::None:
        return -1;
    }
}

u32 translateBlendFactorCommon(gx::BlendModeFactor factor)
{
    switch (factor) {
    case gx::BlendModeFactor::zero:
        return GL_ZERO;
    case gx::BlendModeFactor::one:
        return GL_ONE;
    case gx::BlendModeFactor::src_a:
        return GL_SRC_ALPHA;
    case gx::BlendModeFactor::inv_src_a:
        return GL_ONE_MINUS_SRC_ALPHA;
    case gx::BlendModeFactor::dst_a:
        return GL_DST_ALPHA;
    case gx::BlendModeFactor::inv_dst_a:
        return GL_ONE_MINUS_DST_ALPHA;
    default:
        throw "";
    }
}

u32 translateBlendSrcFactor(gx::BlendModeFactor factor)
{
    switch (factor) {
	case gx::BlendModeFactor::src_c:
        return GL_DST_COLOR;
    case gx::BlendModeFactor::inv_src_c:
        return GL_ONE_MINUS_DST_COLOR;
    default:
        return translateBlendFactorCommon(factor);
    }
}

u32 translateBlendDstFactor(gx::BlendModeFactor factor)
{
    switch (factor) {
    case gx::BlendModeFactor::src_c:
        return GL_SRC_COLOR;
    case gx::BlendModeFactor::inv_src_c:
        return GL_ONE_MINUS_SRC_COLOR;
    default:
        return translateBlendFactorCommon(factor);
    }
}

u32 translateCompareType(gx::Comparison compareType)
{
    switch (compareType) {
	case gx::Comparison::NEVER:
        return GL_NEVER;
    case gx::Comparison::LESS:
        return GL_LESS;
    case gx::Comparison::EQUAL:
        return GL_EQUAL;
    case gx::Comparison::LEQUAL:
        return GL_LEQUAL;
    case gx::Comparison::GREATER:
        return GL_GREATER;
    case gx::Comparison::NEQUAL:
        return GL_NOTEQUAL;
    case gx::Comparison::GEQUAL:
        return GL_GEQUAL;
    case gx::Comparison::ALWAYS:
        return GL_ALWAYS;
    }
}


void translateGfxMegaState(MegaState& megaState, GXMaterial& material)
{
	megaState.cullMode = translateCullMode(material.mat.getMaterialData().cullMode);
	// megaState.depthWrite = material.ropInfo.depthWrite;
	// megaState.depthCompare = material.ropInfo.depthTest ? reverseDepthForCompareMode(translateCompareType(material.ropInfo.depthFunc)) : GfxCompareMode.ALWAYS;
	megaState.frontFace = FrontFace::CW;

	const auto blendMode = material.mat.getMaterialData().blendMode;
	if (blendMode.type == gx::BlendModeType::none) {
		// megaState.blendMode = GL_FUNC_ADD;
		megaState.blendSrcFactor = GL_ONE;
		megaState.blendDstFactor = GL_ZERO;
	}
	else if (blendMode.type == gx::BlendModeType::blend) {
		// megaState.blendMode = GL_FUNC_ADD;
		megaState.blendSrcFactor = translateBlendSrcFactor(blendMode.source);
		megaState.blendDstFactor = translateBlendDstFactor(blendMode.dest);
	}
	else if (blendMode.type == gx::BlendModeType::subtract) {
		// megaState.blendMode = GL_FUNC_REVERSE_SUBTRACT;
		megaState.blendSrcFactor = GL_ONE;
		megaState.blendDstFactor = GL_ONE;
	}
	else if (blendMode.type == gx::BlendModeType::logic) {
		printf("LOGIC mode is unsupported.\n");
	}
}

gx::ColorSelChanApi getRasColorChannelID(gx::ColorSelChanApi v)
{
	switch (v) {
	case gx::ColorSelChanApi::color0:
	case gx::ColorSelChanApi::alpha0:
	case gx::ColorSelChanApi::color0a0:
		return gx::ColorSelChanApi::color0a0;
	case gx::ColorSelChanApi::color1:
	case gx::ColorSelChanApi::alpha1:
	case gx::ColorSelChanApi::color1a1:
		return gx::ColorSelChanApi::color1a1;
	case gx::ColorSelChanApi::ind_alpha:
		return gx::ColorSelChanApi::ind_alpha;
	case gx::ColorSelChanApi::normalized_ind_alpha:
		return gx::ColorSelChanApi::normalized_ind_alpha;
	case gx::ColorSelChanApi::zero:
	case gx::ColorSelChanApi::null:
		return gx::ColorSelChanApi::zero;
	default:
		throw "whoops";
	}
}

} // namespace libcube
