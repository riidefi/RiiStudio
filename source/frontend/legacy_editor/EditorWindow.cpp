#include "EditorWindow.hpp"
#include <core/3d/i3dmodel.hpp>                            // lib3d::Scene
#include <frontend/applet.hpp>                             // core::Applet
#include <frontend/legacy_editor/views/Outliner.hpp>       // MakeHistoryList
#include <frontend/legacy_editor/views/PropertyEditor.hpp> // MakeOutliner
#include <frontend/level_editor/ViewCube.hpp>
#include <frontend/renderer/Renderer.hpp> // Renderer
#include <frontend/widgets/HistoryListWidget.hpp>
#include <imcxx/Widgets.hpp>
#include <plate/toolkit/Viewport.hpp>     // plate::tk::Viewport
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_TIMES

#include <plugins/g3d/G3dIo.hpp>
#include <plugins/j3d/J3dIo.hpp>

namespace riistudio::frontend {

struct HistoryList : public StudioWindow, private HistoryListWidget {
  HistoryList(auto doCommit, auto doUndo, auto doRedo, auto doCursor,
              auto doSize)
      : StudioWindow("History"), mCommit(doCommit), mUndo(doUndo),
        mRedo(doRedo), mCursor(doCursor), mSize(doSize) {
    setClosable(false);
  }

  void draw_() override { drawHistoryList(); }
  void commit_() override { mCommit(); }
  void undo_() override { mUndo(); }
  void redo_() override { mRedo(); }
  int cursor_() override { return mCursor(); }
  int size_() override { return mSize(); }

  std::function<void()> mCommit, mUndo, mRedo;
  std::function<int()> mCursor, mSize;
};
class RenderTest : public StudioWindow {
public:
  RenderTest(const libcube::Scene& host);
  void draw_() override;

  // TODO: Figure out how to do this concurrently
  void precache();

  std::string hide_mat;
  u32 hide_mat_countdown = 0;

private:
  void drawViewCube();

  // Components
  plate::tk::Viewport mViewport;
  Renderer mRenderer;
};
RenderTest::RenderTest(const libcube::Scene& host)
    : StudioWindow("Viewport"), mRenderer(&host) {
  setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
  setClosable(false);
}

void RenderTest::precache() {
  rsl::info("Precaching render state");
  mRenderer.precache();
}

void RenderTest::draw_() {
  auto bounds = ImGui::GetWindowSize();

  std::string impl_hide_mat;

  // TODO: FPS-dependent value
  if (hide_mat_countdown > 0) {
    // 3 flickers = 6 state transitions per second
    if (((hide_mat_countdown / 10) & 1)) {
      impl_hide_mat = hide_mat;
    }
    --hide_mat_countdown;
  }

  if (mViewport.begin(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y))) {
    mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y),
                     impl_hide_mat);
    drawViewCube();
    mViewport.end();
  }
}

void RenderTest::drawViewCube() {
  const bool view_updated =
      lvl::DrawViewCube(mViewport.mLastResolution.width,
                        mViewport.mLastResolution.height, mRenderer.mViewMtx);
  auto& cam = mRenderer.mSettings.mCameraController;
  if (view_updated) {
    frontend::SetCameraControllerToMatrix(cam, mRenderer.mViewMtx);
  }
}

static void BuildSelect(auto&& mSelection, auto&& _selected, auto&& folder) {
  if (mSelection.mActive->collectionOf == folder.low) {
    for (auto& x : folder) {
      if (mSelection.isSelected(&x)) {
        _selected.insert(&x);
      }
    }
  }
}
static std::set<kpi::IObject*> GatherSelected(auto&& mSelection,
                                              g3d::Collection& root) {
  std::set<kpi::IObject*> _selected;
  if (mSelection.mActive) {
    BuildSelect(mSelection, _selected, root.getModels());
    BuildSelect(mSelection, _selected, root.getTextures());
    BuildSelect(mSelection, _selected, root.getAnim_Srts());
    for (auto& x : root.getModels()) {
      BuildSelect(mSelection, _selected, x.getMaterials());
      BuildSelect(mSelection, _selected, x.getBones());
      BuildSelect(mSelection, _selected, x.getMeshes());
      BuildSelect(mSelection, _selected, x.getBuf_Pos());
      BuildSelect(mSelection, _selected, x.getBuf_Nrm());
      BuildSelect(mSelection, _selected, x.getBuf_Clr());
      BuildSelect(mSelection, _selected, x.getBuf_Uv());
    }
  }
  return _selected;
}
static std::set<kpi::IObject*> GatherSelected(auto&& mSelection,
                                              j3d::Collection& root) {
  std::set<kpi::IObject*> _selected;
  if (mSelection.mActive) {
    BuildSelect(mSelection, _selected, root.getModels());
    BuildSelect(mSelection, _selected, root.getTextures());
    for (auto& x : root.getModels()) {
      BuildSelect(mSelection, _selected, x.getMaterials());
      BuildSelect(mSelection, _selected, x.getBones());
      BuildSelect(mSelection, _selected, x.getMeshes());
    }
  }
  return _selected;
}
static std::string getFileShort(const std::string& path) {
  const auto path_ = path.substr(path.rfind("\\") + 1);

  return path_.substr(path_.rfind("/") + 1);
}

void BRRESEditor::init() {
  // Don't require selection reset on first element
  mDocument.commit(mSelection, false);
  for (auto& tex : mDocument.getRoot().getTextures()) {
    mIconManager.propagateIcon(&tex);
  }

  auto draw_image_icon = [&](const lib3d::Texture* tex, u32 dim) {
    mIconManager.drawImageIcon(tex, dim);
  };
  auto post = [&]() { mDocument.commit(mSelection, true); };
  auto commit_ = [&](bool b) { mDocument.commit(mSelection, b); };
  // mActive must be stable
  auto _get_selection = [&]() {
    return GatherSelected(mSelection, mDocument.getRoot());
  };
  auto _get_active = [&]() { return mSelection.mActive; };
  mPropertyEditor =
      MakePropertyEditor(mDocument.getHistory(), mDocument.getRoot(),
                         _get_selection, _get_active, draw_image_icon);
  mPropertyEditor->mParent = this;
  {
    auto commit_ = [&]() {
      mDocument.getHistory().commit(mDocument.getRoot(), &mSelection, false);
    };
    auto undo_ = [&]() {
      mDocument.getHistory().undo(mDocument.getRoot(), mSelection);
    };
    auto redo_ = [&]() {
      mDocument.getHistory().redo(mDocument.getRoot(), mSelection);
    };
    auto cursor_ = [&]() { return mDocument.getHistory().cursor(); };
    auto size_ = [&]() { return mDocument.getHistory().size(); };
    mHistoryList =
        std::make_unique<HistoryList>(commit_, undo_, redo_, cursor_, size_);
    mHistoryList->mParent = this;
  }
  mOutliner = MakeOutliner(mDocument.getRoot(), mSelection, draw_image_icon,
                           post, commit_);
  mOutliner->mParent = this;
  mRenderTest = std::make_unique<RenderTest>(mDocument.getRoot());
  mRenderTest->mParent = this;
}

// We handle the Dockspace manually (disabling it in an error state)
BRRESEditor::BRRESEditor(std::string path)
    : StudioWindow(getFileShort(path), DockSetting::None), mDocument(nullptr),
      mPath(path) {}
BRRESEditor::BRRESEditor(std::span<const u8> span, const std::string& path)
    : StudioWindow(getFileShort(path), DockSetting::None), mDocument(nullptr),
      mPath(path) {
  auto out = std::make_unique<g3d::Collection>();
  oishii::BinaryReader reader(span, path, std::endian::big);
  kpi::LightIOTransaction trans;
  std::string total;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    total += msg + "\n";
    rsl::error(msg);
    Message emsg(message_class, std::string(domain), std::string(message_body));
    mLoadErrorMessages.push_back(emsg);
  };
  g3d::ReadBRRES(*out, reader, trans);
  if (trans.state != kpi::TransactionState::Complete) {
    mErrorState = true;
    return;
  }
  attach(std::move(out));
}
BRRESEditor::~BRRESEditor() = default;
ImGuiID BRRESEditor::buildDock(ImGuiID root_id) {
  ImGuiID next = root_id;
  ImGuiID dock_right_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 0.3f, nullptr, &next);
  ImGuiID dock_left_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.3f, nullptr, &next);
  ImGuiID dock_left_down_id = ImGui::DockBuilderSplitNode(
      dock_left_id, ImGuiDir_Down, 0.2f, nullptr, &dock_left_id);

  ImGui::DockBuilderDockWindow(idIfyChild("Outliner").c_str(), dock_left_id);
  ImGui::DockBuilderDockWindow(idIfyChild("History").c_str(),
                               dock_left_down_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Property Editor").c_str(),
                               dock_right_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Viewport").c_str(), next);
  return next;
}
void BRRESEditor::draw_() {
  if (mErrorState) {
    mLoadErrors.Draw(mLoadErrorMessages);
    return;
  }
  drawDockspace();
  if (!mLoadErrorMessages.empty()) {
    auto name = idIfyChild("Warnings");
    bool open = true;
    ImGui::SetNextWindowSize(ImVec2(900.0f, 500.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(name.c_str(), &open)) {
      mLoadErrors.Draw(mLoadErrorMessages);
      if (ImGui::Button("OK")) {
        open = false;
      }
    }
    ImGui::End();
    if (!open) {
      mLoadErrorMessages.clear();
    }
  }
  detachClosedChildren();

  mPropertyEditor->draw();
  mHistoryList->draw();

  auto* active = mSelection.mActive;

  mOutliner->draw();

  if (mSelection.mActive != active) {
    if (auto* g = dynamic_cast<lib3d::Material*>(active)) {
      mRenderTest->hide_mat = g->getName();
      mRenderTest->hide_mat_countdown = 60;
    }
  }

  mRenderTest->draw();

  // TODO: Only affect active window
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mDocument.undo(mSelection);
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mDocument.redo(mSelection);
    }
  }
}

void BMDEditor::init() {
  // Don't require selection reset on first element
  mDocument.commit(mSelection, false);
  for (auto& tex : mDocument.getRoot().getTextures()) {
    mIconManager.propagateIcon(&tex);
  }

  auto draw_image_icon = [&](const lib3d::Texture* tex, u32 dim) {
    mIconManager.drawImageIcon(tex, dim);
  };
  auto post = [&]() { mDocument.commit(mSelection, true); };
  auto commit_ = [&](bool b) { mDocument.commit(mSelection, b); };
  // mActive must be stable
  auto _get_selection = [&]() {
    return GatherSelected(mSelection, mDocument.getRoot());
  };
  auto _get_active = [&]() { return mSelection.mActive; };
  mPropertyEditor =
      MakePropertyEditor(mDocument.getHistory(), mDocument.getRoot(),
                         _get_selection, _get_active, draw_image_icon);
  mPropertyEditor->mParent = this;
  {
    auto commit_ = [&]() {
      mDocument.getHistory().commit(mDocument.getRoot(), &mSelection, false);
    };
    auto undo_ = [&]() {
      mDocument.getHistory().undo(mDocument.getRoot(), mSelection);
    };
    auto redo_ = [&]() {
      mDocument.getHistory().redo(mDocument.getRoot(), mSelection);
    };
    auto cursor_ = [&]() { return mDocument.getHistory().cursor(); };
    auto size_ = [&]() { return mDocument.getHistory().size(); };
    mHistoryList =
        std::make_unique<HistoryList>(commit_, undo_, redo_, cursor_, size_);
    mHistoryList->mParent = this;
  }
  mOutliner = MakeOutliner(mDocument.getRoot(), mSelection, draw_image_icon,
                           post, commit_);
  mOutliner->mParent = this;
  mRenderTest = std::make_unique<RenderTest>(mDocument.getRoot());
  mRenderTest->mParent = this;
}
void BRRESEditor::saveAsImpl(std::string path) {
  oishii::Writer writer(std::endian::big);
  auto& col = mDocument.getRoot();
  if (auto ok = g3d::WriteBRRES(col, writer); !ok) {
    rsl::ErrorDialogFmt("Failed to rebuild BRRES: {}", ok.error());
    return;
  }
  writer.saveToDisk(path);
}

// We handle the Dockspace manually (disabling it in an error state)
BMDEditor::BMDEditor(std::string path)
    : StudioWindow(getFileShort(path), DockSetting::None), mDocument(nullptr),
      mPath(path) {}
BMDEditor::BMDEditor(std::span<const u8> span, const std::string& path)
    : StudioWindow(getFileShort(path), DockSetting::None), mDocument(nullptr),
      mPath(path) {
  auto out = std::make_unique<j3d::Collection>();
  oishii::BinaryReader reader(span, path, std::endian::big);
  kpi::LightIOTransaction trans;
  std::string total;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    total += msg + "\n";
    rsl::error(msg);
    Message emsg(message_class, std::string(domain), std::string(message_body));
    mLoadErrorMessages.push_back(emsg);
  };
  auto ok = j3d::ReadBMD(*out, reader, trans);
  if (!ok) {
    rsl::error(ok.error());
    mErrorState = true;
    return;
  }
  if (trans.state != kpi::TransactionState::Complete) {
    rsl::ErrorDialog(total);
    return;
  }
  attach(std::move(out));
}
BMDEditor::~BMDEditor() = default;
ImGuiID BMDEditor::buildDock(ImGuiID root_id) {
  ImGuiID next = root_id;
  ImGuiID dock_right_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 0.3f, nullptr, &next);
  ImGuiID dock_left_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.3f, nullptr, &next);
  ImGuiID dock_left_down_id = ImGui::DockBuilderSplitNode(
      dock_left_id, ImGuiDir_Down, 0.2f, nullptr, &dock_left_id);

  ImGui::DockBuilderDockWindow(idIfyChild("Outliner").c_str(), dock_left_id);
  ImGui::DockBuilderDockWindow(idIfyChild("History").c_str(),
                               dock_left_down_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Property Editor").c_str(),
                               dock_right_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Viewport").c_str(), next);
  return next;
}
void BMDEditor::draw_() {
  if (mErrorState) {
    mLoadErrors.Draw(mLoadErrorMessages);
    return;
  }
  drawDockspace();
  if (!mLoadErrorMessages.empty()) {
    auto name = idIfyChild("Warnings");
    bool open = true;
    ImGui::SetNextWindowSize(ImVec2(900.0f, 500.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(name.c_str(), &open)) {
      mLoadErrors.Draw(mLoadErrorMessages);
      if (ImGui::Button("OK")) {
        open = false;
      }
    }
    ImGui::End();
    if (!open) {
      mLoadErrorMessages.clear();
    }
  }
  detachClosedChildren();

  mPropertyEditor->draw();
  mHistoryList->draw();

  auto* active = mSelection.mActive;

  mOutliner->draw();

  if (mSelection.mActive != active) {
    if (auto* g = dynamic_cast<lib3d::Material*>(active)) {
      mRenderTest->hide_mat = g->getName();
      mRenderTest->hide_mat_countdown = 60;
    }
  }

  mRenderTest->draw();

  // TODO: Only affect active window
  if (ImGui::GetIO().KeyCtrl) {
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
      mDocument.undo(mSelection);
    } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
      mDocument.redo(mSelection);
    }
  }
}
void BMDEditor::saveAsImpl(std::string path) {
  if (path.ends_with(".bdl")) {
    path.resize(path.size() - 4);
    path += ".bmd";
  }
  oishii::Writer writer(std::endian::big);
  auto& col = mDocument.getRoot();
  if (auto ok = j3d::WriteBMD(col, writer); !ok) {
    rsl::ErrorDialogFmt("Failed to rebuild BMD: {}", ok.error());
    return;
  }
  writer.saveToDisk(path);
}

} // namespace riistudio::frontend
