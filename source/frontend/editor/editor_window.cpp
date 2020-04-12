#include "editor_window.hpp"

#include <random>
#include <regex>

#include "Renderer.hpp"
#include "kit/Image.hpp"
#include "kit/Viewport.hpp"
#include <vendor/fa5/IconsFontAwesome5.h>

#include <plugins/gc/Export/Material.hpp>

#ifdef _WIN32
#include <vendor/glfw/glfw3.h>
#else
#include <SDL.h>

#include <emscripten.h>

#include <emscripten/html5.h>
#endif

#include <core/kpi/PropertyView.hpp>
#include <core/kpi/RichNameManager.hpp>

#include <algorithm>
#include <plugins/gc/Encoder/ImagePlatform.hpp>

#include <plugins/gc/Util/TextureExport.hpp>
#include <vendor/FileDialogues.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <vendor/stb_image.h>

#undef near

// Web security...
#ifdef __EMSCRIPTEN__
extern bool gPointerLock;
#endif

namespace riistudio::frontend {

inline bool ends_with(const std::string &value, const std::string &ending) {
  return ending.size() <= value.size() &&
         std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct GenericCollectionOutliner : public StudioWindow {
  GenericCollectionOutliner(kpi::IDocumentNode &host)
      : StudioWindow("Outliner"), mHost(host) {}

  struct ImTFilter : public ImGuiTextFilter {
    bool test(const std::string &str) const noexcept {
      return PassFilter(str.c_str());
    }
  };
  struct RegexFilter {
    bool test(const std::string &str) const noexcept {
      try {
        std::regex match(mFilt);
        return std::regex_search(str, match);
      } catch (std::exception e) {
        return false;
      }
    }
    void Draw() {
#ifdef _WIN32
      char buf[128]{};
      memcpy_s(buf, 128, mFilt.c_str(), mFilt.length());
      ImGui::InputText(ICON_FA_SEARCH " (regex)", buf, 128);
      mFilt = buf;
#endif
    }

    std::string mFilt;
  };
  using TFilter = RegexFilter;

  //! @brief Return the number of resources in the source that pass the filter.
  //!
  std::size_t calcNumFiltered(const kpi::FolderData &sampler,
                              const TFilter *filter = nullptr) const noexcept {
    // If no data, empty
    if (sampler.size() == 0)
      return 0;

    // If we don't have a filter, everything is included.
    if (!filter)
      return sampler.size();

    std::size_t nPass = 0;

    for (u32 i = 0; i < sampler.size(); ++i)
      if (filter->test(sampler[i]->getName().c_str()))
        ++nPass;

    return nPass;
  }

  //! @brief Format the title in the "<header> (<number of resources>)" format.
  //!
  std::string formatTitle(const kpi::FolderData &sampler,
                          const TFilter *filter = nullptr) const {
    const auto rich = kpi::RichNameManager::getInstance().getRich(
        &sampler.at<kpi::IDocumentNode>(0));
    const std::string icon_plural = rich.getIconPlural();
    const std::string exposed_name = rich.getNamePlural();
    return std::string(icon_plural + "  " + exposed_name + " (" +
                       std::to_string(calcNumFiltered(sampler, filter)) + ")");
  }

  void drawFolder(kpi::FolderData &sampler, const kpi::IDocumentNode &host,
                  const std::string &key) noexcept {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (!ImGui::TreeNode(formatTitle(sampler, &mFilter).c_str()))
      return;

    //	if (ImGui::Button("Shuffle")) {
    //		static std::default_random_engine rng{};
    //		std::shuffle(std::begin(sampler), std::end(sampler), rng);
    //	}

    // A filter tree for multi selection. Prevents inclusion of unfiltered data
    // with SHIFT clicks.
    std::vector<int> filtered;

    int justSelectedId = -1;
    // Relative to filter vector.
    std::size_t justSelectedFilteredIdx = -1;
    // Necessary to filter out clicks on already selected items.
    bool justSelectedAlreadySelected = false;
    // Prevent resetting when SHIFT is unpressed with arrow keys.
    bool thereWasAClick = false;

    // Draw the tree
    for (int i = 0; i < sampler.size(); ++i) {
      auto &nodeAt = sampler[i];
      const std::string cur_name = nodeAt->getName();

      if (nodeAt->children.empty() && !mFilter.test(cur_name)) {
        continue;
      }

      filtered.push_back(i);

      // Whether or not this node is already selected.
      // Selections from other windows will carry over.
      bool curNodeSelected = sampler.isSelected(i);

      bool bSelected =
          ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected);
      ImGui::SameLine();

      thereWasAClick = ImGui::IsItemClicked();
      bool focused = ImGui::IsItemFocused();

      const auto rich =
          kpi::RichNameManager::getInstance().getRich(nodeAt.get());

      if (ImGui::TreeNodeEx(
              std::to_string(i).c_str(),
              ImGuiTreeNodeFlags_DefaultOpen |
                  (nodeAt->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0),
              (rich.getIconSingular() + " " + cur_name).c_str())) {
        // NodeDrawer::drawNode(*node.get());

        drawRecursive(*nodeAt.get());

        // If there waas a click above, we need to ignore the focus below.
        // Assume only one item can be clicked.
        if (thereWasAClick ||
            (focused && justSelectedId == -1 && !curNodeSelected)) {
          // If it was already selected, we may need to reprocess for ctrl
          // clicks followed by shift clicks
          justSelectedAlreadySelected = curNodeSelected;
          justSelectedId = i;
          justSelectedFilteredIdx = filtered.size() - 1;
        }

        ImGui::TreePop();
      }
    }
    ImGui::TreePop();

    // If nothing new was selected, no new processing needs to occur.
    if (justSelectedId == -1)
      return;

    // Currently, nothing for ctrl + shift or ctrl + art.
    // If both are pressed, SHIFT takes priority.
    const ImGuiIO &io = ImGui::GetIO();
    bool shiftPressed = io.KeyShift;
    bool ctrlPressed = io.KeyCtrl;

    if (shiftPressed) {
      // Transform last selection index into filtered space.
      std::size_t lastSelectedFilteredIdx = -1;
      for (std::size_t i = 0; i < filtered.size(); ++i) {
        if (filtered[i] == sampler.getActiveSelection())
          lastSelectedFilteredIdx = i;
      }
      if (lastSelectedFilteredIdx == -1) {
        // If our last selection is no longer in filtered space, we can just
        // treat it as a control key press.
        shiftPressed = false;
        ctrlPressed = true;
      } else {
#undef min
#undef max
        // Iteration must occur in filtered space to prevent selection of
        // occluded nodes.
        const std::size_t a =
            std::min(justSelectedFilteredIdx, lastSelectedFilteredIdx);
        const std::size_t b =
            std::max(justSelectedFilteredIdx, lastSelectedFilteredIdx);

        for (std::size_t i = a; i <= b; ++i)
          sampler.select(filtered[i]);
      }
    }

    // If the control key was pressed, we add to the selection if necessary.
    if (ctrlPressed && !shiftPressed) {
      // If already selected, no action necessary - just for id update.
      if (!justSelectedAlreadySelected)
        sampler.select(justSelectedId);
      else if (thereWasAClick)
        sampler.deselect(justSelectedId);
    } else if (!ctrlPressed && !shiftPressed &&
               (thereWasAClick ||
                justSelectedId != sampler.getActiveSelection())) {
      // Replace selection
      sampler.clearSelection();
      sampler.select(justSelectedId);
    }

    // Set our last selection index, for shift clicks.
    sampler.setActiveSelection(justSelectedId);
  }

  void drawRecursive(kpi::IDocumentNode &host) noexcept {
    for (auto &folder : host.children)
      drawFolder(folder.second, host, folder.first);
  }
  void draw_() noexcept override {
    mFilter.Draw();
    drawRecursive(mHost);
  }

private:
  kpi::IDocumentNode &mHost;
  TFilter mFilter;
};

static const std::vector<std::string> StdImageFilters = {
    "PNG Files", "*.png",     "TGA Files", "*.tga",     "JPG Files",
    "*.jpg",     "BMP Files", "*.bmp",     "All Files", "*",
};

struct TextureEditor : public StudioWindow {
  TextureEditor(kpi::IDocumentNode &host, kpi::History &history)
      : StudioWindow("Texture Preview"), mHost(host), mHistory(history) {
    setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
  }
  void draw_() override {
    auto *osamp = mHost.getFolder<lib3d::Texture>();
    if (!osamp)
      return;
    assert(osamp);
    auto &samp = *osamp;

    auto &tex = samp.at<lib3d::Texture>(samp.getActiveSelection());
    auto &mut = (libcube::Texture &)tex;

    bool resizeAction = false;
    bool reformatOption = false;

    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("Transform")) {
        if (ImGui::Button(ICON_FA_DRAW_POLYGON " Resize")) {
          resizeAction = true;
        }
        if (ImGui::Button(ICON_FA_DRAW_POLYGON " Change format")) {
          reformatOption = true;
        }
        if (ImGui::Button(ICON_FA_SAVE " Export")) {
          auto results = pfd::save_file("Export image", "", StdImageFilters);
          if (!results.result().empty()) {
            const std::string path = results.result();
            libcube::STBImage imgType = libcube::STBImage::PNG;
            if (ends_with(path, ".png")) {
              imgType = libcube::STBImage::PNG;
            } else if (ends_with(path, ".bmp")) {
              imgType = libcube::STBImage::BMP;
            } else if (ends_with(path, ".tga")) {
              imgType = libcube::STBImage::TGA;
            } else if (ends_with(path, ".jpg")) {
              imgType = libcube::STBImage::JPG;
            }

            // Only top LOD
            libcube::writeImageStbRGBA(path.c_str(), imgType, tex.getWidth(),
                                       tex.getHeight(), mImg.mDecodeBuf.data());
          }
        }
        if (ImGui::Button(ICON_FA_FILE " Import")) {
          auto result =
              pfd::open_file("Import image", "", StdImageFilters).result();
          if (!result.empty()) {
            const auto path = result[0];
            int width, height, channels;
            unsigned char *image = stbi_load(path.c_str(), &width, &height,
                                             &channels, STBI_rgb_alpha);
            assert(image);
            tex.setWidth(width);
            tex.setHeight(height);
            mut.setMipmapCount(0);
            mut.resizeData();
            tex.encode(image);
            stbi_image_free(image);
            mHistory.commit(mHost);
            lastTexId = -1;
          }
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    if (resizeAction) {
      ImGui::OpenPopup("Resize");
      resize[0].value = -1;
      resize[1].value = -1;
      resize[0].before = -1;
      resize[1].before = -1;
    } else if (reformatOption) {
      ImGui::OpenPopup("Reformat");
      reformatOpt = -1;
    }
    if (ImGui::BeginPopupModal("Reformat", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      if (reformatOpt == -1) {
        reformatOpt = mut.getTextureFormat();
      }
      ImGui::InputInt("Format", &reformatOpt);
      if (ImGui::Button(ICON_FA_CHECK " Okay")) {
        const auto oldFormat = mut.getTextureFormat();
        mut.setTextureFormat(reformatOpt);
        mut.resizeData();

        libcube::image_platform::transform(
            mut.getData(), mut.getWidth(), mut.getHeight(),
            static_cast<libcube::gx::TextureFormat>(oldFormat),
            static_cast<libcube::gx::TextureFormat>(reformatOpt), mut.getData(),
            mut.getWidth(), mut.getHeight(), mut.getMipmapCount() - 1);
        mHistory.commit(mHost);
        lastTexId = -1;

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();

      if (ImGui::Button(ICON_FA_CROSS " Cancel")) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Resize", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      if (resize[0].before <= 0) {
        resize[0].before = tex.getWidth();
      }
      if (resize[1].before <= 0) {
        resize[1].before = tex.getHeight();
      }

      int dX = 0;
      for (auto &it : resize) {
        auto &other = dX++ ? resize[0] : resize[1];

        int before = it.value;
        ImGui::InputInt(it.name, &it.value, 1, 64);
        ImGui::SameLine();
        ImGui::Checkbox((std::string("Constrained##") + it.name).c_str(),
                        &it.constrained);
        if (it.constrained) {
          other.constrained = false;
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string("Reset##") + it.name).c_str())) {
          it.value = -1;
        }

        if (before != it.value) {
          if (it.constrained) {
            it.value = before;
          } else if (other.constrained) {
            other.value = other.before * it.value / it.before;
          }
        }
      }

      if (resize[0].value <= 0) {
        resize[0].value = tex.getWidth();
      }
      if (resize[1].value <= 0) {
        resize[1].value = tex.getHeight();
      }

      ImGui::Combo("Algorithm", (int *)&resizealgo, "Ultimate\0Lanczos\0");

      if (ImGui::Button(ICON_FA_CHECK " Resize")) {
        printf("Do the resizing..\n");

        const auto oldWidth = mut.getWidth();
        const auto oldHeight = mut.getHeight();

        mut.setWidth(resize[0].value);
        mut.setHeight(resize[1].value);
        mut.resizeData();

        libcube::image_platform::transform(
            mut.getData(), resize[0].value, resize[1].value,
            static_cast<libcube::gx::TextureFormat>(mut.getTextureFormat()),
            std::nullopt, mut.getData(), oldWidth, oldHeight,
            mut.getMipmapCount(),
            static_cast<libcube::image_platform::ResizingAlgorithm>(
                resizealgo));
        mHistory.commit(mHost);
        lastTexId = -1;

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();

      if (ImGui::Button(ICON_FA_CROSS " Cancel")) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    if (lastTexId != samp.getActiveSelection())
      setFromImage(tex);

    mImg.draw();

    if (ImGui::CollapsingHeader("DEBUG")) {
      int width = tex.getWidth();
      ImGui::InputInt("width", &width);
      const_cast<lib3d::Texture &>(tex).setWidth(width);
      int mmCnt = mut.getMipmapCount();
      ImGui::InputInt("Mipmap Count", &mmCnt);
      mut.setMipmapCount(mmCnt);
    }
  }
  void setFromImage(const lib3d::Texture &tex) { mImg.setFromImage(tex); }

  struct ResizeDimension {
    const char *name = "?";
    int before = -1;
    int value = -1;
    bool constrained = true;
  };

  std::array<ResizeDimension, 2> resize{
      ResizeDimension{"Width ", -1, -1, false},
      ResizeDimension{"Height", -1, -1, true}};
  int reformatOpt = -1;
  int resizealgo = 0;
  kpi::IDocumentNode &mHost;
  int lastTexId = -1;
  ImagePreview mImg;
  kpi::History &mHistory;
};
struct RenderTest : public StudioWindow {

  RenderTest(const kpi::IDocumentNode &host)
      : StudioWindow("Viewport"), mHost(host) {
    setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

    const auto *models = host.getFolder<lib3d::Model>();

    if (!models)
      return;

    if (models->size() == 0)
      return;

    model = &models->at<kpi::IDocumentNode>(0);
    mRenderer.prepare(*model, host);
  }
  void draw_() override {

    auto bounds = ImGui::GetWindowSize();

    if (mViewport.begin(static_cast<u32>(bounds.x),
                        static_cast<u32>(bounds.y))) {
      auto *parent = dynamic_cast<EditorWindow *>(mParent);
      static bool showCursor = false; // TODO
      mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y),
                       showCursor);

#ifdef RII_BACKEND_GLFW
      if (mpGlfwWindow != nullptr)
        glfwSetInputMode(mpGlfwWindow, GLFW_CURSOR,
                         showCursor ? GLFW_CURSOR_NORMAL
                                    : GLFW_CURSOR_DISABLED);
#elif defined(RII_BACKEND_SDL) && defined(__EMSCRIPTEN__)
      gPointerLock = !showCursor;
      if (showCursor)
        emscripten_exit_pointerlock();
      else
        emscripten_request_pointerlock(0, EM_TRUE);
        // SDL_SetRelativeMouseMode(!showCursor ? SDL_TRUE : SDL_FALSE);
#endif
      mViewport.end();
    }
  }
  Viewport mViewport;
  Renderer mRenderer;
  const kpi::IDocumentNode *model;
  const kpi::IDocumentNode &mHost; // texture
};

struct HistoryList : public StudioWindow {
  HistoryList(kpi::History &host, kpi::IDocumentNode &root)
      : StudioWindow("History"), mHost(host), mRoot(root) {}
  void draw_() override {
    if (ImGui::Button("Commit " ICON_FA_SAVE)) {
      mHost.commit(mRoot);
    }
    ImGui::SameLine();
    if (ImGui::Button("Undo " ICON_FA_UNDO)) {
      mHost.undo(mRoot);
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo " ICON_FA_REDO)) {
      mHost.redo(mRoot);
    }
    ImGui::BeginChild("Record List");
    for (std::size_t i = 0; i < mHost.size(); ++i) {
      ImGui::Text("(%s) History #%u", i == mHost.cursor() ? "X" : " ",
                  static_cast<u32>(i));
    }
    ImGui::EndChild();
  }

  kpi::History &mHost;
  kpi::IDocumentNode &mRoot;
};

struct PropertyEditor : public StudioWindow {
  PropertyEditor(kpi::History &host, kpi::IDocumentNode &root)
      : StudioWindow("Property Editor"), mHost(host), mRoot(root) {
    auto &mdls = *root.getFolder<lib3d::Model>();
    auto &mdl = mdls[0];
    mMats = mdl->getFolder<libcube::IGCMaterial>();
    mImgs = root.getFolder<lib3d::Texture>();
  }
  void draw_() override {
    auto &manager = kpi::PropertyViewManager::getInstance();

    kpi::IPropertyView *activeTab = nullptr;
    auto &active = mMats->at<kpi::IDocumentNode>(mMats->getActiveSelection());

    std::vector<kpi::IDocumentNode *> mats;
    for (std::size_t i = 0; i < mMats->size(); ++i) {
      if (mMats->isSelected(i))
        mats.push_back(&mMats->at<kpi::IDocumentNode>(i));
    }
    if (mats.empty()) {
      ImGui::Text("No material is selected");
      return;
    }
    auto *_active = dynamic_cast<libcube::IGCMaterial *>(
        (*mMats)[mMats->getActiveSelection()].get());
    ImGui::Text("%s %s (%u)", _active->getName().c_str(),
                mats.size() > 1 ? "..." : "", static_cast<u32>(mats.size()));

    if (ImGui::BeginTabBar("Pane")) {
      int i = 0;
      std::string title;
      manager.forEachView(
          [&](kpi::IPropertyView &view) {
            const bool sel = mActiveTab == i;

            title.clear();
            title += view.getIcon();
            title += " ";
            title += view.getName();

            if (ImGui::BeginTabItem(title.c_str())) {
              mActiveTab = i;
              activeTab = &view;
              ImGui::EndTabItem();
            }

            ++i;
          },
          active);
      ImGui::EndTabBar();
    }

    if (activeTab == nullptr) {
      mActiveTab = 0;

      ImGui::Text("Invalid Pane");
      return;
    }

    activeTab->draw(active, mats, mHost, mRoot);
  }

  kpi::History &mHost;
  kpi::IDocumentNode &mRoot;

  kpi::FolderData *mMats = nullptr;
  kpi::FolderData *mImgs = nullptr;

  int mActiveTab = 0;
};

EditorWindow::EditorWindow(std::unique_ptr<kpi::IDocumentNode> state,
                           const std::string &path)
    : StudioWindow(path.substr(path.rfind("\\") + 1), true),
      mState(std::move(state)), mFilePath(path) {
  mHistory.commit(*mState.get());

  attachWindow(std::make_unique<PropertyEditor>(mHistory, *mState.get()));
  attachWindow(std::make_unique<HistoryList>(mHistory, *mState.get()));
  attachWindow(std::make_unique<GenericCollectionOutliner>(*mState.get()));
  attachWindow(std::make_unique<TextureEditor>(*mState.get(), mHistory));
  attachWindow(std::make_unique<RenderTest>(*mState.get()));
}
void EditorWindow::draw_() {
#ifdef _WIN32
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mHistory.undo(*mState.get());
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mHistory.redo(*mState.get());
    }
  }
#endif
  auto *parent = mParent;

  // if (!parent) return;

  //	if (!showCursor)
  //	{
  //		parent->hideMouse();
  //	}
  //	else
  //	{
  //		parent->showMouse();
  //	}
}
} // namespace riistudio::frontend
