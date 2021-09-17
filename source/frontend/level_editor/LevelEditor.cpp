#include "LevelEditor.hpp"
#include <core/common.h>
#include <core/util/oishii.hpp>
#include <frontend/root.hpp>
#include <librii/glhelper/Util.hpp>
#include <librii/szs/SZS.hpp>
#include <librii/u8/U8.hpp>
#include <optional>
#include <plugins/g3d/G3dIo.hpp>

namespace riistudio::lvl {

static std::optional<Archive> ReadArchive(std::span<const u8> buf) {
  auto expanded = librii::szs::getExpandedSize(buf);
  if (expanded == 0) {
    DebugReport("Failed to grab expanded size\n");
    return std::nullopt;
  }

  std::vector<u8> decoded(expanded);
  auto err = librii::szs::decode(decoded, buf);
  if (err) {
    DebugReport("Failed to decode SZS\n");
    return std::nullopt;
  }

  if (decoded.size() < 4 || decoded[0] != 0x55 || decoded[1] != 0xaa ||
      decoded[2] != 0x38 || decoded[3] != 0x2d) {
    DebugReport("Not a valid archive\n");
    return std::nullopt;
  }

  librii::U8::U8Archive arc;
  if (!librii::U8::LoadU8Archive(arc, decoded)) {
    DebugReport("Failed to read archive\n");
    return std::nullopt;
  }

  Archive n_arc;

  struct Pair {
    Archive* folder;
    u32 sibling_next;
  };
  std::vector<Pair> n_path;

  n_path.push_back(
      Pair{.folder = &n_arc, .sibling_next = arc.nodes[0].folder.sibling_next});
  for (int i = 1; i < arc.nodes.size(); ++i) {
    auto& node = arc.nodes[i];

    if (node.is_folder) {
      auto tmp = std::make_unique<Archive>();
      auto& parent = n_path.back();
      n_path.push_back(
          Pair{.folder = tmp.get(), .sibling_next = node.folder.sibling_next});
      parent.folder->folders.emplace(node.name, std::move(tmp));
    } else {
      const u32 start_pos = node.file.offset;
      const u32 end_pos = node.file.offset + node.file.size;
      assert(node.file.offset + node.file.size <= arc.file_data.size());
      std::vector<u8> vec(arc.file_data.data() + start_pos,
                          arc.file_data.data() + end_pos);
      n_path.back().folder->files.emplace(node.name, std::move(vec));
    }

    while (i + 1 == n_path.back().sibling_next)
      n_path.resize(n_path.size() - 1);
  }
  assert(n_path.empty());

  // Eliminate the period
  if (n_arc.folders.begin()->first == ".") {
    return *n_arc.folders["."];
  }

  return n_arc;
}

struct Reader {
  oishii::DataProvider mData;
  oishii::BinaryReader mReader;

  Reader(std::string path, const std::vector<u8>& data)
      : mData(OishiiReadFile(path, data.data(), data.size())),
        mReader(mData.slice()) {}
};

struct SimpleTransaction {
  SimpleTransaction() {
    trans.callback = [](...) {};
  }

  bool success() const {
    return trans.state == kpi::TransactionState::Complete;
  }

  kpi::LightIOTransaction trans;
};

std::unique_ptr<g3d::Collection> ReadBRRES(const std::vector<u8>& buf,
                                           std::string path) {
  auto result = std::make_unique<g3d::Collection>();

  SimpleTransaction trans;
  Reader reader(path, buf);
  g3d::ReadBRRES(*result, reader.mReader, trans.trans);

  if (!trans.success())
    return nullptr;

  return result;
}

void LevelEditorWindow::openFile(std::span<const u8> buf, std::string path) {
  auto root_arc = ReadArchive(buf);
  if (!root_arc.has_value())
    return;

  mLevel.root_archive = std::move(*root_arc);
  mLevel.og_path = path;

  auto course_model_brres = FindFile(mLevel.root_archive, "course_model.brres");
  if (course_model_brres.has_value()) {
    mCourseModel = ReadBRRES(*course_model_brres, "course_model.brres");
  }

  auto vrcorn_model_brres = FindFile(mLevel.root_archive, "vrcorn_model.brres");
  if (course_model_brres.has_value()) {
    mVrcornModel = ReadBRRES(*vrcorn_model_brres, "vrcorn_model.brres");
  }
}

static std::optional<std::pair<std::string, std::vector<u8>>>
GatherNodes(Archive& arc) {
  std::optional<std::pair<std::string, std::vector<u8>>> clicked;
  for (auto& f : arc.folders) {
    if (ImGui::TreeNode((f.first + "/").c_str())) {
      GatherNodes(*f.second.get());
      ImGui::TreePop();
    }
  }
  for (auto& f : arc.files) {
    if (ImGui::Selectable(f.first.c_str())) {
      clicked = f;
    }
  }

  return clicked;
}

void LevelEditorWindow::draw_() {
  if (ImGui::Begin("Hi")) {
    auto clicked = GatherNodes(mLevel.root_archive);
    if (clicked.has_value()) {
      auto* pParent = frontend::RootWindow::spInstance;
      if (pParent) {
        pParent->dropDirect(clicked->second, clicked->first);
      }
    }
  }
  ImGui::End();

  if (ImGui::Begin("View", nullptr, ImGuiWindowFlags_MenuBar)) {
    auto bounds = ImGui::GetWindowSize();
    if (mViewport.begin(static_cast<u32>(bounds.x),
                        static_cast<u32>(bounds.y))) {
      drawScene(bounds.x, bounds.y);

      mViewport.end();
    }
  }
  ImGui::End();
}

void LevelEditorWindow::drawScene(u32 width, u32 height) {
  mRenderSettings.drawMenuBar();

  if (!mRenderSettings.rend)
    return;

  librii::glhelper::SetGlWireframe(mRenderSettings.wireframe);

  if (mMouseHider.begin_interaction(ImGui::IsWindowFocused())) {
    // TODO: This time step is a rolling average so is not quite accurate
    // frame-by-frame.
    const auto time_step = 1.0f / ImGui::GetIO().Framerate;
    const auto input_state = frontend::buildInputState();
    mRenderSettings.mCameraController.move(time_step, input_state);

    mMouseHider.end_interaction(input_state.clickView);
  }

  // Only configure once we have stuff
  if (mSceneState.getBuffers().opaque.nodes.size())
    frontend::ConfigureCameraControllerByBounds(
        mRenderSettings.mCameraController, mSceneState.computeBounds());

  glm::mat4 projMtx, viewMtx;
  mRenderSettings.mCameraController.mCamera.calcMatrices(width, height, projMtx,
                                                         viewMtx);

  mSceneState.invalidate();

  {
    if (mVrcornModel) {
      mVrcornModel->prepare(mSceneState, *mVrcornModel, projMtx, viewMtx);
    }
    if (mCourseModel) {
      mCourseModel->prepare(mSceneState, *mCourseModel, projMtx, viewMtx);
    }
  }

  mSceneState.buildUniformBuffers();

  librii::glhelper::ClearGlScreen();
  mSceneState.draw();
}

} // namespace riistudio::lvl
