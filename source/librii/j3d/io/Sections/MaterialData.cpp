#define GLM_ENABLE_EXPERIMENTAL
#include <core/util/glm_io.hpp>
#include <librii/j3d/io/Sections.hpp>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>
#include <vendor/glm/gtx/matrix_decompose.hpp>

namespace librii::j3d {

template <>
Result<void> io_wrapper<u8>::onRead(rsl::SafeReader& reader, u8& out) {
  out = TRY(reader.U8());
  return {};
}
template <> void io_wrapper<u8>::onWrite(oishii::Writer& writer, const u8& in) {
  writer.write<u8>(in);
}

template <>
Result<void> io_wrapper<gx::ZMode>::onRead(rsl::SafeReader& reader,
                                           gx::ZMode& out) {
  out.compare = TRY(reader.U8());
  out.function = TRY(reader.Enum8<gx::Comparison>());
  out.update = TRY(reader.U8());
  TRY(reader.U8());
  return {};
}
template <>
void io_wrapper<gx::ZMode>::onWrite(oishii::Writer& writer,
                                    const gx::ZMode& in) {
  writer.write<u8>(in.compare);
  writer.write<u8>(static_cast<u8>(in.function));
  writer.write<u8>(in.update);
  writer.write<u8>(0xff);
}

template <>
Result<void> io_wrapper<gx::AlphaComparison>::onRead(rsl::SafeReader& reader,
                                                     gx::AlphaComparison& c) {
  c.compLeft = TRY(reader.Enum8<gx::Comparison>());
  c.refLeft = TRY(reader.U8());
  c.op = TRY(reader.Enum8<gx::AlphaOp>());
  c.compRight = TRY(reader.Enum8<gx::Comparison>());
  c.refRight = TRY(reader.U8());
  reader.getUnsafe().skip(3);
  return {};
}
template <>
void io_wrapper<gx::AlphaComparison>::onWrite(oishii::Writer& writer,
                                              const gx::AlphaComparison& in) {
  writer.write<u8>(static_cast<u8>(in.compLeft));
  writer.write<u8>(in.refLeft);
  writer.write<u8>(static_cast<u8>(in.op));
  writer.write<u8>(static_cast<u8>(in.compRight));
  writer.write<u8>(static_cast<u8>(in.refRight));
  for (int i = 0; i < 3; ++i)
    writer.write<u8>(0xff);
}

template <>
Result<void> io_wrapper<gx::BlendMode>::onRead(rsl::SafeReader& reader,
                                               gx::BlendMode& c) {
  c.type = TRY(reader.Enum8<gx::BlendModeType>());
  c.source = TRY(reader.Enum8<gx::BlendModeFactor>());
  c.dest = TRY(reader.Enum8<gx::BlendModeFactor>());
  c.logic = TRY(reader.Enum8<gx::LogicOp>());
  return {};
}
template <>
void io_wrapper<gx::BlendMode>::onWrite(oishii::Writer& writer,
                                        const gx::BlendMode& in) {
  writer.write<u8>(static_cast<u8>(in.type));
  writer.write<u8>(static_cast<u8>(in.source));
  writer.write<u8>(static_cast<u8>(in.dest));
  writer.write<u8>(static_cast<u8>(in.logic));
}

template <>
Result<void> io_wrapper<gx::Color>::onRead(rsl::SafeReader& reader,
                                           gx::Color& out) {
  out.r = TRY(reader.U8());
  out.g = TRY(reader.U8());
  out.b = TRY(reader.U8());
  out.a = TRY(reader.U8());
  return {};
}
template <>
void io_wrapper<gx::Color>::onWrite(oishii::Writer& writer,
                                    const gx::Color& in) {
  writer.write<u8>(in.r);
  writer.write<u8>(in.g);
  writer.write<u8>(in.b);
  writer.write<u8>(in.a);
}

template <>
Result<void> io_wrapper<gx::ChannelControl>::onRead(rsl::SafeReader& reader,
                                                    gx::ChannelControl& out) {
  out.enabled = TRY(reader.U8());
  out.Material = static_cast<gx::ColorSource>(TRY(reader.U8()));
  out.lightMask = static_cast<gx::LightID>(TRY(reader.U8()));
  out.diffuseFn = static_cast<gx::DiffuseFunction>(TRY(reader.U8()));
  // TODO Data loss?
  constexpr const std::array<gx::AttenuationFunction, 4> cvt = {
      gx::AttenuationFunction::None, gx::AttenuationFunction::Specular,
      gx::AttenuationFunction::None2, gx::AttenuationFunction::Spotlight};
  const auto in = TRY(reader.U8());
  out.attenuationFn = in < cvt.size() ? cvt[in] : (gx::AttenuationFunction)(in);
  out.Ambient = static_cast<gx::ColorSource>(TRY(reader.U8()));

  TRY(reader.U16());
  return {};
}
template <>
void io_wrapper<gx::ChannelControl>::onWrite(oishii::Writer& writer,
                                             const gx::ChannelControl& in) {
  writer.write<u8>(in.enabled);
  writer.write<u8>(static_cast<u8>(in.Material));
  writer.write<u8>(static_cast<u8>(in.lightMask));
  writer.write<u8>(static_cast<u8>(in.diffuseFn));
  assert(static_cast<u8>(in.attenuationFn) <=
         static_cast<u8>(gx::AttenuationFunction::None2));
  constexpr const std::array<u8, 4> cvt{
      1, // spec
      3, // spot
      0, // none
      2  // none
  };
  writer.write<u8>(cvt[static_cast<u8>(in.attenuationFn)]);
  writer.write<u8>(static_cast<u8>(in.Ambient));
  writer.write<u16>(-1);
}

template <>
Result<void> io_wrapper<gx::ColorS10>::onRead(rsl::SafeReader& reader,
                                              gx::ColorS10& out) {
  out.r = TRY(reader.S16());
  out.g = TRY(reader.S16());
  out.b = TRY(reader.S16());
  out.a = TRY(reader.S16());
  return {};
}
template <>
void io_wrapper<gx::ColorS10>::onWrite(oishii::Writer& writer,
                                       const gx::ColorS10& in) {
  writer.write<s16>(in.r);
  writer.write<s16>(in.g);
  writer.write<s16>(in.b);
  writer.write<s16>(in.a);
}

template <>
Result<void> io_wrapper<NBTScale>::onRead(rsl::SafeReader& reader,
                                          NBTScale& c) {
  c.enable = static_cast<bool>(TRY(reader.U8()));
  reader.getUnsafe().skip(3);
  c.scale = TRY(readVec3(reader));
  return {};
}
template <>
void io_wrapper<NBTScale>::onWrite(oishii::Writer& writer, const NBTScale& in) {
  writer.write<u8>(in.enable);
  for (int i = 0; i < 3; ++i)
    writer.write<u8>(-1);
  writer.write<f32>(in.scale.x);
  writer.write<f32>(in.scale.y);
  writer.write<f32>(in.scale.z);
}

// J3D specific
template <>
Result<void> io_wrapper<gx::TexCoordGen>::onRead(rsl::SafeReader& reader,
                                                 gx::TexCoordGen& ctx) {
  ctx.func = static_cast<gx::TexGenType>(TRY(reader.U8()));
  ctx.sourceParam = static_cast<gx::TexGenSrc>(TRY(reader.U8()));

  ctx.matrix = static_cast<gx::TexMatrix>(TRY(reader.U8()));
  TRY(reader.U8());
  // Assume no postmatrix
  ctx.normalize = false;
  ctx.postMatrix = gx::PostTexMatrix::Identity;
  return {};
}
template <>
void io_wrapper<gx::TexCoordGen>::onWrite(oishii::Writer& writer,
                                          const gx::TexCoordGen& in) {
  writer.write<u8>(static_cast<u8>(in.func));
  writer.write<u8>(static_cast<u8>(in.sourceParam));
  writer.write<u8>(static_cast<u8>(in.matrix));
  writer.write<u8>(-1);
}

struct J3DMappingMethodDecl {
  using Method = libcube::GCMaterialData::CommonMappingMethod;
  enum class Class { Standard, Basic, Old };

  Method _method;
  Class _class;

  bool operator==(const J3DMappingMethodDecl& rhs) const = default;
};

static std::array<J3DMappingMethodDecl, 12> J3DMappingMethods{
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::Standard,
                         J3DMappingMethodDecl::Class::Standard}, // 0
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::EnvironmentMapping,
                         J3DMappingMethodDecl::Class::Basic}, // 1
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::ProjectionMapping,
                         J3DMappingMethodDecl::Class::Basic}, // 2
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::ViewProjectionMapping,
                         J3DMappingMethodDecl::Class::Basic}, // 3
    J3DMappingMethodDecl{
        J3DMappingMethodDecl::Method::Standard,
        J3DMappingMethodDecl::Class::Standard}, // 4 Not supported
    J3DMappingMethodDecl{
        J3DMappingMethodDecl::Method::Standard,
        J3DMappingMethodDecl::Class::Standard}, // 5 Not supported
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::EnvironmentMapping,
                         J3DMappingMethodDecl::Class::Old}, // 6
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::EnvironmentMapping,
                         J3DMappingMethodDecl::Class::Standard}, // 7
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::ProjectionMapping,
                         J3DMappingMethodDecl::Class::Standard}, // 8
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::ViewProjectionMapping,
                         J3DMappingMethodDecl::Class::Standard}, // 9
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::ManualEnvironmentMapping,
                         J3DMappingMethodDecl::Class::Old}, // a
    J3DMappingMethodDecl{J3DMappingMethodDecl::Method::ManualEnvironmentMapping,
                         J3DMappingMethodDecl::Class::Standard} // b
};

template <>
Result<void>
io_wrapper<MaterialData::TexMatrix>::onRead(rsl::SafeReader& reader,
                                            MaterialData::TexMatrix& c) {
  c.projection = TRY(reader.Enum8<gx::TexGenType>());
  // FIXME:
  //	assert(c.projection == gx::TexGenType::Matrix2x4 || c.projection ==
  // gx::TexGenType::Matrix3x4);

  u8 mappingMethod = TRY(reader.U8());
  bool maya = mappingMethod & 0x80;
  mappingMethod &= ~0x80;

  c.transformModel = maya ? Material::CommonTransformModel::Maya
                          : Material::CommonTransformModel::Default;

  if (mappingMethod >= J3DMappingMethods.size()) {
    reader.getUnsafe().warnAt(
        "Invalid mapping method: Enumeration ends at 0xB.", reader.tell() - 1,
        reader.tell());
    mappingMethod = 0;
  } else if (mappingMethod == 4 || mappingMethod == 5) {
    reader.getUnsafe().warnAt(
        "This mapping method has yet to be seen in the wild: "
        "Enumeration ends at 0xB.",
        reader.tell() - 1, reader.tell());
  }

  const auto& method_decl = J3DMappingMethods[mappingMethod];
  switch (method_decl._class) {
  case J3DMappingMethodDecl::Class::Basic:
    c.option = Material::CommonMappingOption::DontRemapTextureSpace;
    break;
  case J3DMappingMethodDecl::Class::Old:
    c.option = Material::CommonMappingOption::KeepTranslation;
    break;
  default:
    break;
  }

  c.method = method_decl._method;
  TRY(reader.U16());
  glm::vec3 origin = TRY(readVec3(reader));
  if (origin != glm::vec3(0.5f, 0.5f, 0.5f)) {
    reader.getUnsafe().warnAt(
        std::format(
            "Warning: Origin is ({}, {}, {}). Normally (0.5, 0.5, 0.5).",
            origin.x, origin.y, origin.z)
            .c_str(),
        reader.tell() - 12, reader.tell());
  }

  c.scale = TRY(readVec2(reader));
  c.rotate = static_cast<f32>(TRY(reader.S16())) * glm::pi<f32>() / (f32)0x7FFF;
  TRY(reader.U16());
  c.translate = TRY(readVec2(reader));
  for (auto& f : c.effectMatrix)
    f = TRY(reader.F32());
  return {};
}
template <>
void io_wrapper<MaterialData::TexMatrix>::onWrite(
    oishii::Writer& writer, const MaterialData::TexMatrix& in) {
  writer.write<u8>(static_cast<u8>(in.projection));

  u8 mappingMethod = 0;
  bool maya = in.transformModel == Material::CommonTransformModel::Maya;

  J3DMappingMethodDecl::Class _class = J3DMappingMethodDecl::Class::Standard;

  switch (in.option) {
  case Material::CommonMappingOption::DontRemapTextureSpace:
    _class = J3DMappingMethodDecl::Class::Basic;
    break;
  case Material::CommonMappingOption::KeepTranslation:
    _class = J3DMappingMethodDecl::Class::Old;
    break;
  default:
    break;
  }

  const auto mapIdx =
      std::find(J3DMappingMethods.begin(), J3DMappingMethods.end(),
                J3DMappingMethodDecl{in.method, _class});

  if (mapIdx != J3DMappingMethods.end())
    mappingMethod = mapIdx - J3DMappingMethods.begin();

  writer.write<u8>(static_cast<u8>(mappingMethod) | (maya ? 0x80 : 0));
  writer.write<u16>(-1);
  writer.write<f32>(0.5f); // TODO
  writer.write<f32>(0.5f);
  writer.write<f32>(0.5f);
  writer.write<f32>(in.scale.x);
  writer.write<f32>(in.scale.y);
  writer.write<u16>(
      (u16)roundf(static_cast<f32>(in.rotate * (f32)0x7FFF / glm::pi<f32>())));
  writer.write<u16>(-1);
  writer.write<f32>(in.translate.x);
  writer.write<f32>(in.translate.y);
  for (const auto f : in.effectMatrix)
    writer.write<f32>(f);
}

template <>
Result<void> io_wrapper<SwapSel>::onRead(rsl::SafeReader& reader, SwapSel& c) {
  c.colorChanSel = TRY(reader.U8());
  c.texSel = TRY(reader.U8());
  TRY(reader.U16());
  return {};
}
template <>
void io_wrapper<SwapSel>::onWrite(oishii::Writer& writer, const SwapSel& in) {
  writer.write<u8>(in.colorChanSel);
  writer.write<u8>(in.texSel);
  writer.write<u16>(-1);
}

template <>
Result<void> io_wrapper<gx::SwapTableEntry>::onRead(rsl::SafeReader& reader,
                                                    gx::SwapTableEntry& c) {
  c.r = TRY(reader.Enum8<gx::ColorComponent>());
  c.g = TRY(reader.Enum8<gx::ColorComponent>());
  c.b = TRY(reader.Enum8<gx::ColorComponent>());
  c.a = TRY(reader.Enum8<gx::ColorComponent>());
  return {};
}
template <>
void io_wrapper<gx::SwapTableEntry>::onWrite(oishii::Writer& writer,
                                             const gx::SwapTableEntry& in) {
  writer.write<u8>(static_cast<u8>(in.r));
  writer.write<u8>(static_cast<u8>(in.g));
  writer.write<u8>(static_cast<u8>(in.b));
  writer.write<u8>(static_cast<u8>(in.a));
}

template <>
Result<void> io_wrapper<TevOrder>::onRead(rsl::SafeReader& reader,
                                          TevOrder& c) {
  c.texCoord = TRY(reader.U8());
  c.texMap = TRY(reader.U8());
  auto t = reader.Enum8<gx::ColorSelChanApi>();
  t = t.value_or(gx::ColorSelChanApi::null);
  c.rasOrder = TRY(t);
  TRY(reader.U8());
  return {};
}
template <>
void io_wrapper<TevOrder>::onWrite(oishii::Writer& writer, const TevOrder& in) {
  writer.write<u8>(in.texCoord);
  writer.write<u8>(in.texMap);
  writer.write<u8>(static_cast<u8>(in.rasOrder));
  writer.write<u8>(-1);
}

template <>
Result<void> io_wrapper<gx::TevStage>::onRead(rsl::SafeReader& reader,
                                              gx::TevStage& c) {
  const auto unk1 = TRY(reader.U8());
  // Assumed to be TevOp (see attributedlandmeteoritec.bmd)
  (void)unk1;
  if (unk1 != 0xff) {
    auto msg = std::format("Unexpected unk1 value 0x{:02x} (presumed to be "
                           "GXTevOp). Expected 0xff.",
                           unk1);
    reader.getUnsafe().warnAt(msg.c_str(), reader.tell() - 1, reader.tell());
  }
  // assert(unk1 == 0xff);
  c.colorStage.a = TRY(reader.Enum8<gx::TevColorArg>());
  c.colorStage.b = TRY(reader.Enum8<gx::TevColorArg>());
  c.colorStage.c = TRY(reader.Enum8<gx::TevColorArg>());
  c.colorStage.d = TRY(reader.Enum8<gx::TevColorArg>());

  c.colorStage.formula = TRY(reader.Enum8<gx::TevColorOp>());
  // See icemountainplanet.bdl for an example of an invalid TevBias
  auto b = reader.Enum8<gx::TevBias>();
  c.colorStage.bias = b.value_or(gx::TevBias::zero);
  c.colorStage.scale = TRY(reader.Enum8<gx::TevScale>());
  c.colorStage.clamp = TRY(reader.Bool8());
  c.colorStage.out = TRY(reader.Enum8<gx::TevReg>());

  c.alphaStage.a = TRY(reader.Enum8<gx::TevAlphaArg>());
  c.alphaStage.b = TRY(reader.Enum8<gx::TevAlphaArg>());
  c.alphaStage.c = TRY(reader.Enum8<gx::TevAlphaArg>());
  c.alphaStage.d = TRY(reader.Enum8<gx::TevAlphaArg>());

  c.alphaStage.formula = TRY(reader.Enum8<gx::TevAlphaOp>());
  c.alphaStage.bias = TRY(reader.Enum8<gx::TevBias>());
  c.alphaStage.scale = TRY(reader.Enum8<gx::TevScale>());
  c.alphaStage.clamp = TRY(reader.Bool8());
  c.alphaStage.out = TRY(reader.Enum8<gx::TevReg>());

  const auto unk2 = TRY(reader.U8());
  // See seesawmovenuta.bmd
  (void)unk2;
  if (unk2 != 0xff) {
    auto msg =
        std::format("Unexpected unk2 value 0x{:02x}. Expected 0xff.", unk2);
    reader.getUnsafe().warnAt(msg.c_str(), reader.tell() - 1, reader.tell());
  }
  // EXPECT(unk2 == 0xff);
  return {};
}
template <>
void io_wrapper<gx::TevStage>::onWrite(oishii::Writer& writer,
                                       const gx::TevStage& in) {
  writer.write<u8>(0xff);
  writer.write<u8>(static_cast<u8>(in.colorStage.a));
  writer.write<u8>(static_cast<u8>(in.colorStage.b));
  writer.write<u8>(static_cast<u8>(in.colorStage.c));
  writer.write<u8>(static_cast<u8>(in.colorStage.d));

  writer.write<u8>(static_cast<u8>(in.colorStage.formula));
  writer.write<u8>(static_cast<u8>(in.colorStage.bias));
  writer.write<u8>(static_cast<u8>(in.colorStage.scale));
  writer.write<u8>(static_cast<u8>(in.colorStage.clamp));
  writer.write<u8>(static_cast<u8>(in.colorStage.out));

  writer.write<u8>(static_cast<u8>(in.alphaStage.a));
  writer.write<u8>(static_cast<u8>(in.alphaStage.b));
  writer.write<u8>(static_cast<u8>(in.alphaStage.c));
  writer.write<u8>(static_cast<u8>(in.alphaStage.d));

  writer.write<u8>(static_cast<u8>(in.alphaStage.formula));
  writer.write<u8>(static_cast<u8>(in.alphaStage.bias));
  writer.write<u8>(static_cast<u8>(in.alphaStage.scale));
  writer.write<u8>(static_cast<u8>(in.alphaStage.clamp));
  writer.write<u8>(static_cast<u8>(in.alphaStage.out));
  writer.write<u8>(0xff);
}

template <>
Result<void> io_wrapper<Fog>::onRead(rsl::SafeReader& reader, Fog& f) {
  f.type = TRY(reader.Enum8<librii::gx::FogType>());
  f.enabled = TRY(reader.U8());
  f.center = TRY(reader.U16());
  f.startZ = TRY(reader.F32());
  f.endZ = TRY(reader.F32());
  f.nearZ = TRY(reader.F32());
  f.farZ = TRY(reader.F32());
  io_wrapper<gx::Color>::onRead(reader, f.color);
  for (auto& e : f.rangeAdjTable)
    e = TRY(reader.F32());
  return {};
}
template <>
void io_wrapper<Fog>::onWrite(oishii::Writer& writer,
                              const librii::j3d::Fog& in) {
  writer.write<u8>(static_cast<u8>(in.type));
  writer.write<u8>(in.enabled);
  writer.write<u16>(in.center);
  writer.write<f32>(in.startZ);
  writer.write<f32>(in.endZ);
  writer.write<f32>(in.nearZ);
  writer.write<f32>(in.farZ);
  io_wrapper<gx::Color>::onWrite(writer, in.color);
  for (const auto x : in.rangeAdjTable)
    writer.write<u16>(x);
}

template <>
Result<void> io_wrapper<MaterialData::J3DSamplerData>::onRead(
    rsl::SafeReader& reader, MaterialData::J3DSamplerData& sampler) {
  sampler.btiId = TRY(reader.U16());
  return {};
}
template <>
void io_wrapper<MaterialData::J3DSamplerData>::onWrite(
    oishii::Writer& writer, const Material::J3DSamplerData& sampler) {
  writer.write<u16>(sampler.btiId);
}

template <>
Result<void> io_wrapper<Indirect>::onRead(rsl::SafeReader& reader,
                                          Indirect& c) {
  const auto enabled = TRY(reader.U8());
  c.enabled = enabled != 0;
  c.nIndStage = TRY(reader.U8());
  if (c.nIndStage > 4)
    reader.getUnsafe().warnAt("Invalid stage count", reader.tell() - 1,
                              reader.tell());
  TRY(reader.U16());

  EXPECT(enabled <= 1 && c.nIndStage <= 4);
  EXPECT(!c.enabled || c.nIndStage);

  for (auto& e : c.tevOrder) {
    // TODO: Switched? Confirm this
    e.refCoord = TRY(reader.U8());
    e.refMap = TRY(reader.U8());
    TRY(reader.U16());
  }
  for (auto& e : c.texMtx) {
    glm::mat2x3 m_raw;

    m_raw[0][0] = TRY(reader.F32());
    m_raw[0][1] = TRY(reader.F32());
    m_raw[0][2] = TRY(reader.F32());
    m_raw[1][0] = TRY(reader.F32());
    m_raw[1][1] = TRY(reader.F32());
    m_raw[1][2] = TRY(reader.F32());
    e.quant = TRY(reader.U8());
    reader.getUnsafe().skip(3);

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose((glm::mat4)m_raw, scale, rotation, translation, skew,
                   perspective);

    // assert(skew == glm::vec3(0.0f) && perspective ==
    // glm::vec4(0.0f));

    e.scale = {scale.x, scale.y};
    e.rotate = glm::degrees(glm::eulerAngles(rotation).z);
    e.trans = {translation.x, translation.y};
  }
  for (auto& e : c.texScale) {
    e.U =
        static_cast<gx::IndirectTextureScalePair::Selection>(TRY(reader.U8()));
    e.V =
        static_cast<gx::IndirectTextureScalePair::Selection>(TRY(reader.U8()));
    reader.getUnsafe().skip(2);
  }
  int i = 0;
  for (auto& e : c.tevStage) {
    u8 id = TRY(reader.U8());
    if (i < c.nIndStage) {
      EXPECT(id == i);
    }
    e.format = static_cast<gx::IndTexFormat>(TRY(reader.U8()));
    e.bias = static_cast<gx::IndTexBiasSel>(TRY(reader.U8()));
    e.matrix = static_cast<gx::IndTexMtxID>(TRY(reader.U8()));
    e.wrapU = static_cast<gx::IndTexWrap>(TRY(reader.U8()));
    e.wrapV = static_cast<gx::IndTexWrap>(TRY(reader.U8()));
    e.addPrev = TRY(reader.U8());
    e.utcLod = TRY(reader.U8());
    e.alpha = static_cast<gx::IndTexAlphaSel>(TRY(reader.U8()));
    reader.getUnsafe().skip(3);

    ++i;
  }
  return {};
}
template <>
void io_wrapper<Indirect>::onWrite(oishii::Writer& writer, const Indirect& c) {
  writer.write<u8>(c.enabled);
  writer.write<u8>(c.nIndStage);
  writer.write<u16>(-1);

  for (const auto& e : c.tevOrder) {
    // TODO: switched?
    writer.write<u8>(e.refCoord);
    writer.write<u8>(e.refMap);
    writer.write<u16>(-1);
  }
  for (const auto& e : c.texMtx) {
    glm::mat4 out(1.0f);

    out = glm::scale(out, {
                              e.scale.x,
                              e.scale.y,
                              1.0f,
                          });
    out = glm::rotate(out, e.rotate, {0.0f, 1.0f, 0.0f});
    out = glm::translate(out, {e.trans.x, e.trans.y, 0.0f});

    f32 aa = out[0][0];
    f32 ab = out[0][1];
    f32 ac = out[0][2];
    f32 ba = out[1][0];
    f32 bb = out[1][1];
    f32 bc = out[1][2];
    writer.write<f32>(aa);
    writer.write<f32>(ab);
    writer.write<f32>(ac);
    writer.write<f32>(ba);
    writer.write<f32>(bb);
    writer.write<f32>(bc);

    writer.write<u8>(e.quant);
    for (int i = 0; i < 3; ++i)
      writer.write<u8>(-1);
  }
  for (int i = 0; i < c.texScale.size(); ++i) {
    writer.write<u8>(static_cast<u8>(c.texScale[i].U));
    writer.write<u8>(static_cast<u8>(c.texScale[i].V));
    writer.write<u16>(-1);
  }
  int i = 0;
  for (const auto& e : c.tevStage) {
    writer.write<u8>(i < c.nIndStage ? i : 0);
    writer.write<u8>(static_cast<u8>(e.format));
    writer.write<u8>(static_cast<u8>(e.bias));
    writer.write<u8>(static_cast<u8>(e.matrix));
    writer.write<u8>(static_cast<u8>(e.wrapU));
    writer.write<u8>(static_cast<u8>(e.wrapV));
    writer.write<u8>(static_cast<u8>(e.addPrev));
    writer.write<u8>(static_cast<u8>(e.utcLod));
    writer.write<u8>(static_cast<u8>(e.alpha));
    for (int j = 0; j < 3; ++j)
      writer.write<u8>(-1);
  }
}

template <>
Result<void> io_wrapper<gx::CullMode>::onRead(rsl::SafeReader& reader,
                                              gx::CullMode& out) {
  out = TRY(reader.Enum32<gx::CullMode>());
  return {};
}
template <>
void io_wrapper<gx::CullMode>::onWrite(oishii::Writer& writer,
                                       const gx::CullMode& cm) {
  writer.write<u32>(static_cast<u32>(cm));
}

} // namespace librii::j3d
