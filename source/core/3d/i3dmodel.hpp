#pragma once

#include <core/kpi/Node.hpp>
#include <vendor/glm/glm.hpp>

#include <array>
#include <core/common.h>
#include <string>

#include <vendor/fa5/IconsFontAwesome5.h>

// TODO: Interdependency
#include <frontend/editor/renderer/UBOBuilder.hpp>
#include <vendor/glm/glm.hpp>

struct MegaState {
  u32 cullMode = -1;
  u32 depthWrite;
  u32 depthCompare;
  u32 frontFace;

  u32 blendMode;
  u32 blendSrcFactor;
  u32 blendDstFactor;
};

namespace riistudio::lib3d {
enum class Coverage { Unsupported, Read, ReadWrite };

template <typename Feature> struct TPropertySupport {

  Coverage supports(Feature f) const noexcept {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    if (static_cast<u64>(f) >= static_cast<u64>(Feature::Max))
      return Coverage::Unsupported;

    return registration[static_cast<u64>(f)];
  }
  bool canRead(Feature f) const noexcept {
    return supports(f) == Coverage::Read || supports(f) == Coverage::ReadWrite;
  }
  bool canWrite(Feature f) const noexcept {
    return supports(f) == Coverage::ReadWrite;
  }
  void setSupport(Feature f, Coverage s) {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    if (static_cast<u64>(f) < static_cast<u64>(Feature::Max))
      registration[static_cast<u64>(f)] = s;
  }

  Coverage& operator[](Feature f) noexcept {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    return registration[static_cast<u64>(f)];
  }
  Coverage operator[](Feature f) const noexcept {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    return registration[static_cast<u64>(f)];
  }

private:
  std::array<Coverage, static_cast<u64>(Feature::Max)> registration;
};

enum class I3DFeature { Bone, Max };
enum class BoneFeatures {
  // -- Standard features: XForm, Hierarchy. Here for read-only access
  SRT,
  Hierarchy,
  // -- Optional features
  StandardBillboards, // J3D
  ExtendedBillboards, // G3D
  AABB,
  BoundingSphere,
  SegmentScaleCompensation, // Maya
                            // Not exposed currently:
                            //	ModelMatrix,
                            //	InvModelMatrix
  Max
};
struct AABB {
  glm::vec3 min, max;

  bool operator==(const AABB& rhs) const {
    return min == rhs.min && max == rhs.max;
  }
};
struct SRT3 {
  glm::vec3 scale;
  glm::vec3 rotation;
  glm::vec3 translation;

  bool operator==(const SRT3& rhs) const {
    return scale == rhs.scale && rotation == rhs.rotation &&
           translation == rhs.translation;
  }
  bool operator!=(const SRT3& rhs) const { return !operator==(rhs); }
};
struct TexSRT {
  glm::vec2 scale;
  f32 rotation;
  glm::vec2 translation;

  bool operator==(const TexSRT& rhs) const {
    return scale == rhs.scale && rotation == rhs.rotation &&
           translation == rhs.translation;
  }
  bool operator!=(const TexSRT& rhs) const { return !operator==(rhs); }
};

struct Bone {
  // // PX_TYPE_INFO_EX("3D Bone", "3d_bone", "3D::Bone", ICON_FA_BONE,
  // ICON_FA_BONE);
  virtual s64 getId() { return -1; }
  virtual void setName(const std::string& name) = 0;
  inline virtual void copy(lib3d::Bone& to) const {
    to.setSRT(getSRT());
    to.setBoneParent(getBoneParent());
    to.setSSC(getSSC());
    to.setAABB(getAABB());
    to.setBoundingRadius(getBoundingRadius());
    // TODO: The rest
    for (int i = 0; i < getNumDisplays(); ++i) {
      const auto d = getDisplay(i);
      to.addDisplay(d);
    }
  }

  virtual Coverage supportsBoneFeature(BoneFeatures f) {
    return Coverage::Unsupported;
  }

  virtual SRT3 getSRT() const = 0;
  virtual void setSRT(const SRT3& srt) = 0;

  virtual s64 getBoneParent() const = 0;
  virtual void setBoneParent(s64 id) = 0;
  virtual u64 getNumChildren() const = 0;
  virtual s64 getChild(u64 idx) const = 0;
  virtual s64 addChild(s64 child) = 0;
  virtual s64 setChild(u64 idx, s64 id) = 0;
  inline s64 removeChild(u64 idx) { return setChild(idx, -1); }

  virtual AABB getAABB() const = 0;
  virtual void setAABB(const AABB& v) = 0;

  virtual float getBoundingRadius() const = 0;
  virtual void setBoundingRadius(float v) = 0;

  virtual bool getSSC() const = 0;
  virtual void setSSC(bool b) = 0;

  struct Display {
    u32 matId = 0;
    u32 polyId = 0;
    u8 prio = 0;

    bool operator==(const Display& rhs) const {
      return matId == rhs.matId && polyId == rhs.polyId && prio == rhs.prio;
    }
  };
  virtual u64 getNumDisplays() const = 0;
  virtual Display getDisplay(u64 idx) const = 0;
  virtual void addDisplay(const Display& d) = 0;
  virtual void setDisplay(u64 idx, const Display& d) = 0;

  inline glm::mat4 calcSrtMtx(const lib3d::SRT3& srt) const {
    glm::mat4 dst(1.0f);

    //	dst = glm::translate(dst, srt.translation);
    //	dst = dst * glm::eulerAngleZYX(glm::radians(srt.rotation.x),
    //glm::radians(srt.rotation.y), glm::radians(srt.rotation.z)); 	return
    //glm::scale(dst, srt.scale);

    float sinX = sin(glm::radians(srt.rotation.x)),
          cosX = cos(glm::radians(srt.rotation.x));
    float sinY = sin(glm::radians(srt.rotation.y)),
          cosY = cos(glm::radians(srt.rotation.y));
    float sinZ = sin(glm::radians(srt.rotation.z)),
          cosZ = cos(glm::radians(srt.rotation.z));

    dst[0][0] = srt.scale.x * (cosY * cosZ);
    dst[0][1] = srt.scale.x * (sinZ * cosY);
    dst[0][2] = srt.scale.x * (-sinY);
    dst[0][3] = 0.0;
    dst[1][0] = srt.scale.y * (sinX * cosZ * sinY - cosX * sinZ);
    dst[1][1] = srt.scale.y * (sinX * sinZ * sinY + cosX * cosZ);
    dst[1][2] = srt.scale.y * (sinX * cosY);
    dst[1][3] = 0.0;
    dst[2][0] = srt.scale.z * (cosX * cosZ * sinY + sinX * sinZ);
    dst[2][1] = srt.scale.z * (cosX * sinZ * sinY - sinX * cosZ);
    dst[2][2] = srt.scale.z * (cosY * cosX);
    dst[2][3] = 0.0;
    dst[3][0] = srt.translation.x;
    dst[3][1] = srt.translation.y;
    dst[3][2] = srt.translation.z;
    dst[3][3] = 1.0;

    return dst;
  }
  virtual kpi::IDocumentNode* getParent() { return nullptr; }
  virtual glm::mat4 calcSrtMtx(kpi::FolderData* bones) const {
    glm::mat4 mdl(1.0f);
    const auto parent = getBoneParent();
    if (parent >= 0 /* && parent != getId() */)
      mdl = bones->at<Bone>(parent).calcSrtMtx(bones);

    return mdl * calcSrtMtx(getSRT());
  }
  inline glm::mat4 calcSrtMtx() {
    assert(getParent() != nullptr);
    if (!getParent())
      return {};

    return calcSrtMtx(getParent()->getFolder<lib3d::Bone>());
  }
};
enum class PixelOcclusion {
  //! The texture does not use alpha.
  //!
  Opaque,

  //! The texture's alpha is binary: a pixel is either fully opaque or
  //! discarded.
  //!
  Stencil,

  //! Pixels may express a range of alpha values for nuanced, translucent
  //! expression.
  //!
  Translucent
};
struct IObserver {
  // TODO: Detach
  virtual void update() {}
};
struct Polygon;
struct Material {
  // // PX_TYPE_INFO("3D Material", "3d_material", "3D::Material");

  virtual std::string getName() const { return "Untitled Material"; }
  virtual void setName(const std::string& name) {}
  virtual s64 getId() const { return -1; }

  virtual bool isXluPass() const { return false; }

  virtual std::pair<std::string, std::string> generateShaders() const = 0;
  // TODO: Interdependency
  virtual void generateUniforms(DelegatedUBOBuilder& builder,
                                const glm::mat4& M, const glm::mat4& V,
                                const glm::mat4& P, u32 shaderId,
                                const std::map<std::string, u32>& texIdMap,
                                const Polygon& poly) const = 0;
  virtual void
  genSamplUniforms(u32 shaderId,
                   const std::map<std::string, u32>& texIdMap) const = 0;
  virtual void onSplice(DelegatedUBOBuilder& builder, const Polygon& poly,
                        u32 id) const {}
  virtual void setMegaState(MegaState& state) const = 0;
  virtual void configure(PixelOcclusion occlusion,
                         std::vector<std::string>& textures) = 0;

  // TODO: Better system..
  void notifyObservers() {
    for (auto* it : observers) {
      it->update();
    }
  }
  mutable std::vector<IObserver*> observers;
};

struct TextureCodec {
  virtual ~TextureCodec() = default;

  virtual u32 getBpp() const = 0;
  virtual u32 getBlockWidth() const = 0;
  virtual u32 getBlockHeight() const = 0;
  virtual void encodeBlock(const u32* rgbaPixels, void* block, int width) = 0;
};

struct Texture {
  // // PX_TYPE_INFO("Texture", "tex", "3D::Texture");

  virtual std::string getName() const { return "Untitled Texture"; }
  virtual void setName(const std::string& name) = 0;
  virtual s64 getId() const { return -1; }

  virtual u32 getDecodedSize(bool mip) const {
    u32 mipMapCount = getMipmapCount();
    if (mip && mipMapCount > 0) {
      u32 w = getWidth();
      u32 h = getHeight();
      u32 total = w * h * 4;
      for (int i = 0; i < mipMapCount; ++i) {
        w >> 1;
        h >> 1;
        total += w * h * 4;
      }
      return total;
    } else {
      return getWidth() * getHeight() * 4;
    }
  }
  virtual u32 getEncodedSize(bool mip) const = 0;
  virtual void decode(std::vector<u8>& out, bool mip) const = 0;

  // 0 -- no mipmap, 1 -- one mipmap; not lod max
  virtual u32 getMipmapCount() const = 0;
  virtual void setMipmapCount(u32 c) = 0;

  virtual u16 getWidth() const = 0;
  virtual void setWidth(u16 width) = 0;
  virtual u16 getHeight() const = 0;
  virtual void setHeight(u16 height) = 0;

  using Occlusion = PixelOcclusion;

  //! @brief Set the image encoder based on the expression profile. Pixels are
  //! not recomputed immediately.
  //!
  //! @param[in] optimizeForSize	If the texture should prefer filesize
  //! over quality when deciding an encoder.
  //! @param[in] color			If the texture is not grayscale.
  //! @param[in] occlusion		The pixel occlusion selection.
  //!
  virtual void setEncoder(bool optimizeForSize, bool color,
                          Occlusion occlusion) = 0;

  //! @brief Encode the texture based on the current encoder, width, height,
  //! etc. and supplied raw data.
  //!
  //! @param[in] rawRGBA
  //!				- Raw pointer to a RGBA32 pixel array.
  //!				- Must be sized width * height * 4.
  //!				- If mipmaps are configured, this must also
  //! include all additional mip levels.
  //!
  virtual void encode(const u8* rawRGBA) = 0;
};

struct Polygon {
  //	// PX_TYPE_INFO("Polygon", "poly", "3D::Polygon");
  virtual ~Polygon() = default;

  virtual bool isVisible() const { return true; }

  virtual void setName(const std::string& name) = 0;

  enum class SimpleAttrib {
    EnvelopeIndex, // u8
    Position,      // vec3
    Normal,        // vec3
    Color0,        // rgba8
    Color1,
    TexCoord0, // vec2
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    Max
  };
  virtual bool hasAttrib(SimpleAttrib attrib) const = 0;
  virtual void setAttrib(SimpleAttrib attrib, bool v) = 0;

  struct SimpleVertex {
    // Not all are real. Use hasAttrib()
    u8 evpIdx; // If read from a GC model, not local to mprim
    glm::vec3 position;
    glm::vec3 normal;
    std::array<glm::vec4, 2> colors;
    std::array<glm::vec2, 8> uvs;
  };

  // For now... (slow api)
  virtual void propogate(VBOBuilder& out) const = 0;

  virtual void addTriangle(std::array<SimpleVertex, 3> tri) = 0;
  //	virtual SimpleVertex getPrimitiveVertex(u64 prim_idx, u64 vtx_idx) = 0;
  //	virtual void setPrimitiveVertex(u64 prim_idx, u64 vtx_idx, const
  // SimpleVertex& vtx) = 0;

  // Call after any change
  virtual void update() {}

  virtual lib3d::AABB getBounds() const = 0;
};
// TODO: This should all be runtime
struct Scene {
  //// PX_TYPE_INFO("Scene", "scene", "3D::Scene");
  virtual ~Scene() = default;
};
struct Model {
  virtual ~Model() = default;
};

} // namespace riistudio::lib3d
