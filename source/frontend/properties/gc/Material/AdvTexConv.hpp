#pragma once

#include <LibBadUIFramework/ActionMenu.hpp>
#include <core/common.h>
#include <frontend/widgets/Lib3dImage.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/rhst/RHSTImporter.hpp>
#include <rsl/Defer.hpp>
#include <rsl/FsDialog.hpp>

namespace libcube::UI {

void OverwriteWithG3dTex(libcube::Texture& tex,
                         const librii::g3d::TextureData& rep);
Result<librii::g3d::TextureData> ReadTexture(const rsl::File& file);

struct AdvancedTextureConverter {
  librii::gx::TextureFormat format{librii::gx::TextureFormat::CMPR};
  int mip_levels{1}; // [1, 10]
  int width{};       // [1, 1024]
  int height{};      // [1, 1024]
  bool constrain{true};
  // For constraint
  int first_width{};
  int first_height{};
  // For encoding -- just the first MIP level
  std::vector<u8> source_data;
  //
  riistudio::g3d::Texture dst_encoded;

  riistudio::frontend::Lib3dCachedImagePreview dst_preview;

  librii::image::ResizingAlgorithm resizer{
      librii::image::ResizingAlgorithm::Lanczos};

  [[nodiscard]] Result<void> Populate(const libcube::Texture& tex) {
    format = tex.getTextureFormat();
    mip_levels = tex.getMipmapCount() + 1;
    width = tex.getWidth();
    height = tex.getHeight();
    constrain = true;
    first_width = width;
    first_height = height;
    source_data.clear();
    tex.decode(source_data, false);
    return ReEncode();
  }

  struct Info {
    u32 size_bytes{};
  };
  [[nodiscard]] Result<Info> GetInfo() {
    EXPECT(width >= 1 && width <= 1024);
    EXPECT(height >= 1 && height <= 1024);
    EXPECT(mip_levels >= 1 && mip_levels <= 10);
    u32 size =
        librii::image::getEncodedSize(width, height, format, mip_levels - 1);
    return Info{
        .size_bytes = size,
    };
  }

  [[nodiscard]] Result<void> SetMipLevels(int x) {
    EXPECT(x >= 1 && x <= 10);
    auto old = mip_levels;
    mip_levels = x;
    auto ok = ReEncode();
    if (!ok) {
      mip_levels = old;
      (void)ReEncode();
    }
    return ok;
  }

  [[nodiscard]] Result<void> SetWidth(int x) {
    EXPECT(x >= 1 && x <= 1024);
    auto old_h = height;
    auto old_w = width;
    if (constrain) {
      float ratio =
          static_cast<float>(first_height) / static_cast<float>(first_width);
      auto maybe_height = static_cast<int>(roundf(x * ratio));
      EXPECT(maybe_height >= 1 && maybe_height <= 1024);
      height = maybe_height;
    }
    width = x;

    auto ok = ReEncode(false);
    if (!ok) {
      width = old_w;
      height = old_h;
      (void)ReEncode(false);
    }

    return ok;
  }
  [[nodiscard]] Result<void> SetHeight(int x) {
    EXPECT(x >= 1 && x <= 1024);
    auto old_h = height;
    auto old_w = width;
    if (constrain) {
      float ratio =
          static_cast<float>(first_width) / static_cast<float>(first_height);
      auto maybe_width = static_cast<int>(roundf(x * ratio));
      EXPECT(maybe_width >= 1 && maybe_width <= 1024);
      width = maybe_width;
    }
    height = x;
    auto ok = ReEncode(false);
    if (!ok) {
      width = old_w;
      height = old_h;
      (void)ReEncode(false);
    }
    return ok;
  }

  // On any setting change
  [[nodiscard]] Result<void> ReEncode(bool should_throw = true) {
    EXPECT(width >= 1 && width <= 1024);
    EXPECT(height >= 1 && height <= 1024);
    EXPECT(mip_levels >= 1 && mip_levels <= 10);
    u32 src_size = first_width * first_height * 4;
    // If mips are also sent, just ignore
    EXPECT(source_data.size() >= src_size);
    std::vector<u8> scratch;
    if (should_throw || (is_power_of_2(width) && is_power_of_2(height) &&
                         width > 4 && height > 4)) {
      TRY(riistudio::rhst::importTextureImpl(
          dst_encoded, source_data, scratch, mip_levels - 1, width, height,
          first_width, first_height, format, resizer));
    }
    // Bust cache
    dst_encoded.nextGenerationId();
    return {};
  }

  [[nodiscard]] Result<void> SetConstrain(bool c) {
    if (c && !constrain) {
      TRY(SetWidth(first_width));
      TRY(SetHeight(first_height));
      constrain = true;
    } else if (!c && constrain) {
      constrain = false;
    }
    return {};
  }
};
enum class TCResult {
  None,
  OK,
  Cancel,
};
Result<TCResult> DrawAdvTexConv(AdvancedTextureConverter& action);

struct ATCAStates {
  struct None {};
  struct Edit {
    AdvancedTextureConverter m_cvtr;
  };
};
using ATCAState = std::variant<ATCAStates::None, ATCAStates::Edit>;

Result<std::optional<AdvancedTextureConverter>>
ATCA_Context(libcube::Texture& tex);

kpi::ChangeType ATCA_Modal(AdvancedTextureConverter& cvtr,
                           libcube::Texture& tex, bool& close);

class AdvTexConvAction
    : public kpi::ActionMenu<libcube::Texture, AdvTexConvAction> {
  using State = ATCAStates;
  ATCAState m_state{State::None{}};

  void error(const std::string& msg) { rsl::ErrorDialog(msg); }

public:
  bool _context(libcube::Texture& tex) {
    auto result = ATCA_Context(tex);
    if (!result) {
      error(result.error());
      return false;
    }
    if (*result) {
      m_state = State::Edit{.m_cvtr = std::move(**result)};
    }
    return false;
  }

  kpi::ChangeType _modal(libcube::Texture& tex) {
    if (m_state.index() == 1) {
      auto& cvtr = std::get_if<State::Edit>(&m_state)->m_cvtr;

      bool cancel = false;
      auto res = ATCA_Modal(cvtr, tex, cancel);
      if (cancel) {
        m_state = State::None{};
      }
      return res;
    }

    return kpi::NO_CHANGE;
  }
};

} // namespace libcube::UI
