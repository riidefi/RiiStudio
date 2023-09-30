#include "AdvTexConv.hpp"

#include <librii/g3d/io/TextureIO.hpp>

#include <rsl/Stb.hpp>

namespace libcube::UI {

void OverwriteWithG3dTex(libcube::Texture& tex,
                         const librii::g3d::TextureData& rep) {
  // Using more general approach to support J3D too
  // static_cast<librii::g3d::TextureData&>(scn.getTextures().add()) =
  //     *replacement;

  tex.setName(rep.name);
  tex.setTextureFormat(rep.format);
  tex.setWidth(rep.width);
  tex.setHeight(rep.height);
  tex.setImageCount(rep.number_of_images);
  tex.setLod(rep.custom_lod, rep.minLod, rep.maxLod);
  tex.setSourcePath(rep.sourcePath);
  tex.resizeData();
  memcpy(tex.getData().data(), rep.data.data(),
         std::min<u32>(tex.getEncodedSize(true), rep.data.size()));
}

Result<librii::g3d::TextureData> ReadTexture(const rsl::File& file) {
  const auto& path = file.path.string();
  if (path.ends_with(".tex0")) {
    auto rep = librii::g3d::ReadTEX0(file.data);
    if (!rep) {
      return std::unexpected("Failed to read .tex0 at \"" + std::string(path) +
                             "\"\n" + rep.error());
    }
    return rep;
  }

  auto image = rsl::stb::load(path);
  if (!image) {
    return std::unexpected(
        std::format("Failed to import texture {}: stb_image didn't "
                    "recognize it or didn't exist.",
                    path));
  }
  std::vector<u8> scratch;
  riistudio::g3d::Texture tex;
  const auto ok = riistudio::rhst::importTexture(
      tex, image->data, scratch, true, 64, 4, image->width, image->height,
      image->channels);
  if (!ok) {
    return std::unexpected(
        std::format("Failed to import texture {}: {}", path, ok.error()));
  }
  tex.setName(file.path.stem().string());
  return tex;
}

Result<TCResult> DrawAdvTexConv(AdvancedTextureConverter& action) {
  // Child 1: no border, enable horizontal scrollbar
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("ChildL",
                      ImVec2(ImGui::GetContentRegionAvail().x - 350.0f,
                             ImGui::GetContentRegionAvail().y - 46.0f),
                      false, window_flags);
    float w = 30.0f;
    float h = 30.0f;
    f32 aspect = (static_cast<f32>(action.dst_encoded.width) /
                  static_cast<f32>(action.dst_encoded.height));
    auto avail = ImGui::GetContentRegionAvail();
    // For LOD slider
    if (action.mip_levels > 1) {
      avail.y -= 50.0f;
    }
    if (aspect >= 1.0f) {
      h = avail.y;
      w = h * aspect;
    } else {
      w = avail.x;
      h = w / aspect;
    }
    f32 x_offset = (avail.x - w) / 2.0f;
    f32 y_offset = (avail.y - h) / 2.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x_offset);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y_offset);
    action.dst_preview.draw(action.dst_encoded, w, h);
    ImGui::EndChild();
  }

  ImGui::SameLine();

  // Child 2: rounded border
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    ImGui::BeginChild("ChildR",
                      ImVec2(0, ImGui::GetContentRegionAvail().y - 46.0f), true,
                      window_flags);
    RSL_DEFER(ImGui::EndChild());

    auto info = TRY(action.GetInfo());
    ImGui::Text("Filesize: %u", info.size_bytes);

    auto fmt =
        imcxx::EnumCombo<librii::gx::TextureFormat>("Format", action.format);
    if (fmt != action.format &&
        fmt == librii::gx::TextureFormat::Extension_RawRGBA32) {
      rsl::ErrorDialog(
          "Warning: Extension_RawRGBA32 is not a real format; it tells "
          "RiiStudio's renderer to pass the raw texture data directly to "
          "the host GPU skipping encoding as a GC texture. This can be "
          "useful for determining if the GC texture codec is at fault for a "
          "bug, but should not be used in actual files.");
    }
    if (fmt != action.format) {
      action.format = fmt;
      if (action.sync) {
        TRY(action.ReEncode());
      }
    }
    int mip = action.mip_levels;
    ImGui::InputInt("Mip Levels", &mip);
    if (mip != action.mip_levels) {
      TRY(action.SetMipLevels(mip));
    }
    auto button = ImVec2{ImGui::GetContentRegionAvail().x / 2.2f, 0};
    int w = action.width;
    ImGui::SetNextItemWidth(button.x);
    ImGui::InputInt("W", &w);
    if (w < 1)
      w = 1;
    if (w > 1024)
      w = 1024;
    int h = action.height;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(button.x);
    ImGui::InputInt("H", &h);
    if (h < 1)
      h = 1;
    if (h > 1024)
      h = 1024;
    if (w != action.width) {
      TRY(action.SetWidth(w));
    } else if (h != action.height) {
      TRY(action.SetHeight(h));
    }
    if (!is_power_of_2(action.width) || !is_power_of_2(action.height)) {
      ImGui::Text("Warning: (%d, %d) is not a power of two, so encoding cannot "
                  "happen!",
                  action.width, action.height);
    }
    // ImGui::SameLine();
    // if (ImGui::Button("Apply", button)) {
    //   TRY(action.ReEncode());
    // }
    bool aspect = action.constrain;
    ImGui::Checkbox("Constrain?", &aspect);
    if (aspect != action.constrain) {
      TRY(action.SetConstrain(aspect));
    }
    action.resizer = imcxx::EnumCombo("Resizing algorithm", action.resizer);
    ImGui::Separator();
    ImGui::Checkbox("Auto-sync", &action.sync);
    if (!action.sync) {
      ImGui::SameLine();
      if (ImGui::Button("Re-encode")) {
        TRY(action.ReEncode());
      }
    }
  }
  ImGui::Separator();

  auto button = ImVec2{75, 0};
  ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - button.x * 2);
  {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("OK", button)) {
      TRY(action.ReEncode());
      return TCResult::OK;
    }
  }
  ImGui::SameLine();
  {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("Cancel", button)) {
      return TCResult::Cancel;
    }
  }

  return TCResult::None;
}

Result<std::optional<AdvancedTextureConverter>>
ATCA_Context(libcube::Texture& tex) {
  if (!ImGui::BeginMenu("Advanced texture editor"_j)) {
    return std::nullopt;
  }
  RSL_DEFER(ImGui::EndMenu());

  if (ImGui::MenuItem("Edit")) {
    AdvancedTextureConverter cvtr{};
    auto ok = cvtr.Populate(tex);
    if (!ok) {
      return std::unexpected("Cannot open advanced texture editor:\n"_j +
                             ok.error());
    }
    return cvtr;
  }
  if (ImGui::MenuItem("Replace")) {
    AdvancedTextureConverter cvtr{};
    const auto file = TRY(rsl::ReadOneFile("Import Path"_j, "",
                                           {
                                               "Image files",
                                               "*.tex0;*.png;*.tga;*.jpg;*.bmp",
                                               "TEX0 Files",
                                               "*.tex0",
                                               "PNG Files",
                                               "*.png",
                                               "TGA Files",
                                               "*.tga",
                                               "JPG Files",
                                               "*.jpg",
                                               "BMP Files",
                                               "*.bmp",
                                               "All Files",
                                               "*",
                                           }));
    auto new_tex = TRY(ReadTexture(file));
    riistudio::g3d::Texture dyn;
    static_cast<librii::g3d::TextureData&>(dyn) = new_tex;
    auto ok = cvtr.Populate(dyn);
    if (!ok) {
      return std::unexpected("Cannot open advanced texture editor:\n"_j +
                             ok.error());
    }
    return cvtr;
  }
  return std::nullopt;
}

kpi::ChangeType ATCA_Modal(AdvancedTextureConverter& cvtr,
                           libcube::Texture& tex, bool& close) {
  close = false;
  auto id = ImGui::GetID("Texture Editor");
  ImGui::OpenPopup(id);
  ImGui::SetNextWindowSize(ImVec2{800.0f, 446.0f}, ImGuiCond_Once);
  if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_NoSavedSettings)) {
    RSL_DEFER(ImGui::EndPopup());
    auto ok = DrawAdvTexConv(cvtr);
    if (!ok) {
      rsl::ErrorDialog(ok.error());
      return kpi::CHANGE;
    }
    switch (*ok) {
    case TCResult::None:
      return kpi::NO_CHANGE;
    case TCResult::OK:
      if (auto ok = cvtr.ReEncode(); !ok) {
        rsl::ErrorDialog(ok.error());
        return kpi::CHANGE;
      }
      cvtr.dst_encoded.name = tex.getName();
      OverwriteWithG3dTex(tex, cvtr.dst_encoded);
      // Bust cache
      tex.nextGenerationId();
      close = true;
      return kpi::CHANGE;
    case TCResult::Cancel:
      close = true;
      return kpi::NO_CHANGE;
    }
  }

  return kpi::NO_CHANGE;
}

} // namespace libcube::UI
