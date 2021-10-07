#include "LevelEditor.hpp"
#include <core/common.h>
#include <core/util/gui.hpp>
#include <core/util/oishii.hpp>
#include <frontend/root.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <librii/glhelper/Util.hpp>
#include <librii/kmp/io/KMP.hpp>
#include <librii/math/srt3.hpp>
#include <librii/szs/SZS.hpp>
#include <librii/u8/U8.hpp>
#include <math.h>
#include <optional>
#include <plugins/g3d/G3dIo.hpp>
#include <vendor/ImGuizmo.h>

#include <core/3d/gl.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/glhelper/ShaderProgram.hpp>
#include <librii/glhelper/VBOBuilder.hpp>

#include <librii/glhelper/Primitives.hpp>

#include "ObjUtil.hpp"

#include "KclUtil.hpp"

#include <glm/gtx/norm.hpp> // glm::distance2

#include <imcxx/Widgets.hpp> // imcxx::Combo

namespace riistudio::lvl {

void DrawRenderOptions(RenderOptions& opt) {
  ImGui::Checkbox("Visual Model", &opt.show_brres);
  ImGui::Checkbox("Collision Model", &opt.show_kcl);

  if (ImGui::BeginCombo("Flags", "wau")) {
    for (int i = 0; i < 32; ++i) {
      if ((opt.target_attr_all & (1 << i)) == 0)
        continue;

      const auto& info = GetKCLType(i);
      ImGui::CheckboxFlags(info.name.c_str(), &opt.attr_mask, 1 << i);
    }

    ImGui::EndCombo();
  }

  opt.xlu_mode = imcxx::Combo("Translucency", opt.xlu_mode, "Fast\0Fancy\0");
  ImGui::SliderFloat("Collision Alpha", &opt.kcl_alpha, 0.0f, 1.0f);
}

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

  assert(arc.nodes.size());

  n_path.push_back(
      Pair{.folder = &n_arc, .sibling_next = arc.nodes[0].folder.sibling_next});
  for (int i = 1; i < arc.nodes.size(); ++i) {
    auto& node = arc.nodes[i];

    if (node.is_folder) {
      auto tmp = std::make_unique<Archive>();
      n_path.push_back(
          Pair{.folder = tmp.get(), .sibling_next = node.folder.sibling_next});
      auto& parent = n_path[n_path.size() - 2];
      parent.folder->folders.emplace(node.name, std::move(tmp));
    } else {
      const u32 start_pos = node.file.offset;
      const u32 end_pos = node.file.offset + node.file.size;
      assert(node.file.offset + node.file.size <= arc.file_data.size());
      std::vector<u8> vec(arc.file_data.data() + start_pos,
                          arc.file_data.data() + end_pos);
      n_path.back().folder->files.emplace(node.name, std::move(vec));
    }

    while (!n_path.empty() && i + 1 == n_path.back().sibling_next)
      n_path.resize(n_path.size() - 1);
  }
  assert(n_path.empty());

  // Eliminate the period
  if (!n_arc.folders.empty() && n_arc.folders.begin()->first == ".") {
    return *n_arc.folders["."];
  }

  return n_arc;
}

static void ProcessArcs(const Archive* arc, std::string_view name,
                        librii::U8::U8Archive& u8) {
  const auto node_index = u8.nodes.size();

  librii::U8::U8Archive::Node node{.is_folder = true,
                                   .name = std::string(name)};
  // Since we write folders first, the parent will always be behind us
  node.folder.parent = u8.nodes.size() - 1;
  node.folder.sibling_next = 0; // Filled in later
  u8.nodes.push_back(node);

  for (auto& data : arc->folders) {
    ProcessArcs(data.second.get(), data.first, u8);
  }

  for (auto& [n, f] : arc->files) {
    {
      librii::U8::U8Archive::Node node{.is_folder = false,
                                       .name = std::string(n)};
      node.file.offset =
          u8.file_data.size(); // Note: relative->abs translation handled later
      node.file.size = f.size();
      u8.nodes.push_back(node);
      u8.file_data.insert(u8.file_data.end(), f.begin(), f.end());
    }
  }

  u8.nodes[node_index].folder.sibling_next = u8.nodes.size();
}

static std::vector<u8> WriteArchive(const Archive& arc) {
  librii::U8::U8Archive u8;
  u8.watermark = {0};

  ProcessArcs(&arc, ".", u8);

  return librii::U8::SaveU8Archive(u8);
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

  // Tentatively allow previewing models we can't rebuild
  if (trans.trans.state == kpi::TransactionState::FailureToSave)
    return result;

  if (!trans.success())
    return nullptr;

  return result;
}

std::unique_ptr<librii::kmp::CourseMap> ReadKMP(const std::vector<u8>& buf,
                                                std::string path) {
  auto result = std::make_unique<librii::kmp::CourseMap>();

  Reader reader(path, buf);
  librii::kmp::readKMP(*result, reader.mData.slice());

  return result;
}

std::vector<u8> WriteKMP(const librii::kmp::CourseMap& map) {
  oishii::Writer writer(0);

  librii::kmp::writeKMP(map, writer);

  return writer.takeBuf();
}

std::unique_ptr<librii::kcol::KCollisionData>
ReadKCL(const std::vector<u8>& buf, std::string path) {
  auto result = std::make_unique<librii::kcol::KCollisionData>();

  Reader reader(path, buf);
  auto res = librii::kcol::ReadKCollisionData(
      *result, reader.mData.slice(), reader.mData.slice().size_bytes());

  if (!res.empty()) {
    printf("Error: %s\n", res.c_str());
    return nullptr;
  }

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

  auto course_kcl = FindFile(mLevel.root_archive, "course.kcl");
  if (course_kcl.has_value()) {
    mCourseKcl = ReadKCL(*course_kcl, "course.kcl");
  }

  if (mCourseKcl) {
    mKclTris.resize(mCourseKcl->prism_data.size());
    for (size_t i = 0; i < mKclTris.size(); ++i) {
      auto& prism = mCourseKcl->prism_data[i];

      mKclTris[i] = {.attr = prism.attribute,
                     .verts = librii::kcol::FromPrism(*mCourseKcl, prism)};
    }

    for (auto& prism : mCourseKcl->prism_data) {
      disp_opts.target_attr_all |= 1 << (prism.attribute & 31);
    }
  }

  auto course_kmp = FindFile(mLevel.root_archive, "course.kmp");
  if (course_kmp.has_value()) {
    mKmp = ReadKMP(*course_kmp, "course.kmp");

    mKmpHistory.push_back(*mKmp);
  }

  auto& cam = mRenderSettings.mCameraController.mCamera;
  cam.mClipMin = 200.0f;
  cam.mClipMax = 1000000.0f;
  mRenderSettings.mCameraController.mSpeed = 15'000.0f;

  if (mKmp && !mKmp->mStartPoints.empty()) {
    auto& start = mKmp->mStartPoints[0];
    cam.mEye = start.position + glm::vec3(0.0f, 10'000.0f, 0.0f);
  }
}

void LevelEditorWindow::saveFile(std::string path) {
  // Update archive cache
  if (mKmp != nullptr)
    mLevel.root_archive.files["course.kmp"] = WriteKMP(*mKmp);

  // Flush archive cache
  auto u8_buf = WriteArchive(mLevel.root_archive);
  auto szs_buf = librii::szs::encodeFast(u8_buf);

  plate::Platform::writeFile(szs_buf, path);
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
  if (ImGui::BeginMenuBar()) {
    if (ImGui::Button("Save")) {
      saveFile("umm.szs");
    }

    ImGui::EndMenuBar();
  }
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

void PushCube(riistudio::lib3d::SceneState& state, glm::mat4 modelMtx,
              glm::mat4 viewMtx, glm::mat4 projMtx) {

  auto& cube = state.getBuffers().opaque.nodes.emplace_back();
  cube.mega_state = {.cullMode = (u32)-1 /* GL_BACK */,
                     .depthWrite = GL_TRUE,
                     .depthCompare = GL_LEQUAL,
                     .frontFace = GL_CW,

                     .blendMode = GL_FUNC_ADD,
                     .blendSrcFactor = GL_ONE,
                     .blendDstFactor = GL_ZERO};
  cube.shader_id = librii::glhelper::GetCubeShader();
  cube.vao_id = librii::glhelper::GetCubeVAO();
  cube.texture_objects.resize(0);

  cube.glBeginMode = GL_TRIANGLES;
  cube.vertex_count = librii::glhelper::GetCubeNumVerts();
  cube.glVertexDataType = GL_UNSIGNED_INT;
  cube.indices = 0; // Offset in VAO

  cube.bound = {};

  glm::mat4 mvp = projMtx * viewMtx * modelMtx;
  librii::gl::UniformSceneParams params{.projection = mvp,
                                        .Misc0 = {69.0f, 0.0f, 0.0f, 0.0f}};

  enum {
    // So UBOBuilder requires the same UBO layout for every use of the same
    // binding point. It also requires there to be 4x binding point 0, 4x
    // binding point 1, etc.. except it doesn't break if we have extra on
    // binding point 0. This is a mess
    UB_SCENEPARAMS_FOR_CUBE_ID = 0
  };

  auto& uniforms =
      cube.uniform_data.emplace_back(librii::gfx::SceneNode::UniformData{
          .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
          .raw_data = {sizeof(params)}});

  uniforms.raw_data.resize(sizeof(params));
  memcpy(uniforms.raw_data.data(), &params, sizeof(params));

  glUniformBlockBinding(
      cube.shader_id, glGetUniformBlockIndex(cube.shader_id, "ub_SceneParams"),
      UB_SCENEPARAMS_FOR_CUBE_ID);

  int query_min = 1024;
  glGetActiveUniformBlockiv(cube.shader_id, UB_SCENEPARAMS_FOR_CUBE_ID,
                            GL_UNIFORM_BLOCK_DATA_SIZE, &query_min);
  // This conflicts with the previous min?

  cube.uniform_mins.push_back(librii::gfx::SceneNode::UniformMin{
      .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
      .min_size = static_cast<u32>(query_min)});
}

const char* gTriShader = R"(
#version 400

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in uint attr_id;

out vec4 fragmentColor;
out float _is_wire;

layout(std140) uniform ub_SceneParams {
    mat4x4 u_Projection;
    vec4 u_Misc0;
};

void main() {	
  gl_Position = u_Projection * vec4(position, 1);
  fragmentColor = color;

  // Wire
  bool is_wire = uint(u_Misc0[2]) == 1;
  if (is_wire) {
    // fragmentColor = vec4(1, 1, 1, 1);
    fragmentColor = vec4(mix(normalize(vec3(color)) * sqrt(3), vec3(1, 1, 1), .5), 1);
  }
  _is_wire = is_wire ? 1.0 : 0.0;


  // Precision hack
  uint low_bits = uint(u_Misc0[0]) & 0xFFFF;
  uint high_bits = uint(u_Misc0[1]) << 16;
  uint attr_mask = low_bits | high_bits;

  fragmentColor.w = u_Misc0[3];

  if ((attr_id & attr_mask) == 0) {
    fragmentColor.w = 0.0;
  }
}
)";
const char* gTriShaderFrag = R"(
#version 330 core

in vec4 fragmentColor;
in float _is_wire;

out vec4 color;

void main() {
  if (fragmentColor.w == 0.0) discard;

  color = fragmentColor;

  // bool is_wire = color == vec4(1, 1, 1, 1);
  bool is_wire = _is_wire != 0.0f;

  if (!gl_FrontFacing && !is_wire)
    color *= vec4(.5, .5, .5, 1.0);

  gl_FragDepth = gl_FragCoord.z * (is_wire ? .99991 : .99999);
}
)";

void PushTriangles(riistudio::lib3d::SceneState& state, glm::mat4 modelMtx,
                   glm::mat4 viewMtx, glm::mat4 projMtx, u32 tri_vao,
                   u32 tri_vao_count, u32 attr_mask, float alpha) {
  static const librii::glhelper::ShaderProgram tri_shader(gTriShader,
                                                          gTriShaderFrag);

  auto& cube = state.getBuffers().translucent.nodes.emplace_back();
  static float poly_ofs = 0.0f;
  // ImGui::InputFloat("Poly_ofs", &poly_ofs);
  static float poly_fact = 0.0f;
  // ImGui::InputFloat("Poly_fact", &poly_fact);
  cube.mega_state = {.cullMode = (u32)-1 /* GL_BACK */,
                     .depthWrite = GL_FALSE,
                     .depthCompare = GL_LEQUAL /* GL_LEQUAL */,
                     .frontFace = GL_CCW,

                     .blendMode = GL_FUNC_ADD,
                     .blendSrcFactor = GL_SRC_ALPHA,
                     .blendDstFactor = GL_ONE_MINUS_SRC_ALPHA,

                     // Prevents z-fighting, always win
                     .poly_offset_factor = poly_fact,
                     .poly_offset_units = poly_ofs};
  cube.shader_id = tri_shader.getId();
  cube.vao_id = tri_vao;
  cube.texture_objects.resize(0);

  cube.glBeginMode = GL_TRIANGLES;
  cube.vertex_count = tri_vao_count;
  cube.glVertexDataType = GL_UNSIGNED_INT;
  cube.indices = 0; // Offset in VAO

  cube.bound = {};

  glm::mat4 mvp = projMtx * viewMtx * modelMtx;
  librii::gl::UniformSceneParams params{
      .projection = mvp,
      .Misc0 = {/* attr_mask_lo */ attr_mask & 0xFFFF,
                /* attr_mask_hi */ attr_mask >> 16,
                /* is_wire */ 0.0f, /* alpha */ alpha}};

  enum {
    // So UBOBuilder requires the same UBO layout for every use of the same
    // binding point. It also requires there to be 4x binding point 0, 4x
    // binding point 1, etc.. except it doesn't break if we have extra on
    // binding point 0. This is a mess
    UB_SCENEPARAMS_FOR_CUBE_ID = 0
  };

  auto& uniforms =
      cube.uniform_data.emplace_back(librii::gfx::SceneNode::UniformData{
          .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
          .raw_data = {sizeof(params)}});

  uniforms.raw_data.resize(sizeof(params));
  memcpy(uniforms.raw_data.data(), &params, sizeof(params));

  glUniformBlockBinding(
      cube.shader_id, glGetUniformBlockIndex(cube.shader_id, "ub_SceneParams"),
      UB_SCENEPARAMS_FOR_CUBE_ID);

  int query_min = 1024;
  glGetActiveUniformBlockiv(cube.shader_id, UB_SCENEPARAMS_FOR_CUBE_ID,
                            GL_UNIFORM_BLOCK_DATA_SIZE, &query_min);
  // This conflicts with the previous min?

  cube.uniform_mins.push_back(librii::gfx::SceneNode::UniformMin{
      .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
      .min_size = static_cast<u32>(query_min)});

  auto& cube_wire = state.getBuffers().translucent.nodes.emplace_back(cube);

  cube_wire.mega_state.fill = librii::gfx::PolygonMode::Line;

  reinterpret_cast<librii::gl::UniformSceneParams*>(
      cube_wire.uniform_data[0].raw_data.data())
      ->Misc0[2] = 1.0f;
}

struct Manipulator {
  bool dgui = true;
  ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;
  bool useSnap = false;
  float st[3];
  float sr[3];
  float ss[3];
  float* snap = st;

  void drawUi(glm::mat4& mx) {
    if (ImGui::IsKeyPressed(65 + 'G' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(65 + 'R' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(65 + 'T' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate",
                           mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate",
                           mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mx), matrixTranslation,
                                          matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation, 3);
    ImGui::InputFloat3("Rt", matrixRotation, 3);
    ImGui::InputFloat3("Sc", matrixScale, 3);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                            matrixScale, glm::value_ptr(mx));

    if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
    if (ImGui::IsKeyPressed(83))
      useSnap = !useSnap;
    ImGui::Checkbox("", &useSnap);
    ImGui::SameLine();
    switch (mCurrentGizmoOperation) {
    case ImGuizmo::TRANSLATE:
      snap = st;
      ImGui::InputFloat3("Snap", st);
      break;
    case ImGuizmo::ROTATE:
      snap = sr;
      ImGui::InputFloat("Angle Snap", sr);
      break;
    case ImGuizmo::SCALE:
      snap = ss;
      ImGui::InputFloat("Scale Snap", ss);
      break;
    default:
      snap = st;
    }
  }

  bool manipulate(glm::mat4& mx, const glm::mat4& viewMtx,
                  const glm::mat4& projMtx) {
    return ImGuizmo::Manipulate(glm::value_ptr(viewMtx),
                                glm::value_ptr(projMtx), mCurrentGizmoOperation,
                                mCurrentGizmoMode, glm::value_ptr(mx), NULL,
                                useSnap ? snap : NULL);
  }

  void drawDebugCube(const glm::mat4& mx, const glm::mat4& viewMtx,
                     const glm::mat4& projMtx) {
    ImGuizmo::DrawCubes(glm::value_ptr(viewMtx), glm::value_ptr(projMtx),
                        glm::value_ptr(mx), 1);
  }
};

void LevelEditorWindow::drawScene(u32 width, u32 height) {
  mRenderSettings.drawMenuBar();

  // if (ImGui::BeginMenuBar()) {
  DrawRenderOptions(disp_opts);
  //   ImGui::EndMenuBar();
  // }

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
  if (mSceneState.getBuffers().opaque.nodes.size()) {
    frontend::ConfigureCameraControllerByBounds(
        mRenderSettings.mCameraController, mSceneState.computeBounds(), 1.0f);
    auto& cam = mRenderSettings.mCameraController.mCamera;
    cam.mClipMin = 200.0f;
    cam.mClipMax = 1000000.0f;
  }

  glm::mat4 projMtx, viewMtx;
  mRenderSettings.mCameraController.mCamera.calcMatrices(width, height, projMtx,
                                                         viewMtx);

  mSceneState.invalidate();

  if (disp_opts.show_brres) {
    if (mVrcornModel) {
      mVrcornModel->prepare(mSceneState, *mVrcornModel, viewMtx, projMtx);
    }
    if (mCourseModel) {
      mCourseModel->prepare(mSceneState, *mCourseModel, viewMtx, projMtx);
    }
  }

  if (mKmp != nullptr) {
    for (auto& pt : mKmp->mRespawnPoints) {
      glm::mat4 modelMtx = librii::math::calcXform(
          {.scale =
               glm::vec3(std::max(std::min(pt.range * 50.0f, 1000.0f), 700.0f)),
           .rotation = pt.rotation,
           .translation = pt.position});
      PushCube(mSceneState, modelMtx, viewMtx, projMtx);
    }
  }

  if (tri_vbo == nullptr) {
    tri_vbo = std::make_unique<librii::glhelper::VBOBuilder>();

    tri_vbo->mPropogating[0].descriptor =
        librii::glhelper::VAOEntry{.binding_point = 0,
                                   .name = "position",
                                   .format = GL_FLOAT,
                                   .size = sizeof(glm::vec3)};
    tri_vbo->mPropogating[1].descriptor =
        librii::glhelper::VAOEntry{.binding_point = 1,
                                   .name = "color",
                                   .format = GL_FLOAT,
                                   .size = sizeof(glm::vec4)};
    tri_vbo->mPropogating[2].descriptor =
        librii::glhelper::VAOEntry{.binding_point = 2,
                                   .name = "attr_id",
                                   .format = GL_UNSIGNED_INT,
                                   .size = sizeof(u32)};

    for (int i = 0; i < 3 * mKclTris.size(); ++i) {
      tri_vbo->mIndices.push_back(static_cast<u32>(tri_vbo->mIndices.size()));
      tri_vbo->pushData(/*binding_point=*/0, mKclTris[i / 3].verts[i % 3]);
      tri_vbo->pushData(
          /*binding_point=*/1,
          glm::vec4(GetKCLColor(mKclTris[i / 3].attr), 1.0f));
      tri_vbo->pushData(/*binding_point=*/2,
                        static_cast<u32>(1 << (mKclTris[i / 3].attr & 31)));
    }
    tri_vbo->build();
  }

  if (disp_opts.show_kcl) {
    // Z sort
    if (tri_vbo != nullptr && disp_opts.xlu_mode == XluMode::Fancy) {
      struct index_tri {
        u32 points[3];

        const glm::vec3& get_point(librii::glhelper::VBOBuilder& tri_vbo,
                                   u32 i) const {
          return reinterpret_cast<const glm::vec3&>(
              tri_vbo.mPropogating[0].data[points[i] * sizeof(glm::vec3)]);
        }

        float get_sphere_dist(librii::glhelper::VBOBuilder& tri_vbo, u32 i,
                              const glm::vec3& eye) const {
          // Don't sqrt
          return glm::distance2(get_point(tri_vbo, i), eye);
        }

        float get_min_sphere_dist(librii::glhelper::VBOBuilder& tri_vbo,
                                  const glm::vec3& view) const {
          return std::min({get_sphere_dist(tri_vbo, 0, view),
                           get_sphere_dist(tri_vbo, 1, view),
                           get_sphere_dist(tri_vbo, 2, view)});
        }

        float get_plane_dist(librii::glhelper::VBOBuilder& tri_vbo, u32 i,
                             const glm::mat4& view) const {
          // We assume all points are closer, or all are further away
          return (view * glm::vec4(get_point(tri_vbo, i), 1.0f)).z;
        }

        float get_min_plane_dist(librii::glhelper::VBOBuilder& tri_vbo,
                                 const glm::mat4& view) const {
          return std::max({get_plane_dist(tri_vbo, 0, view),
                           get_plane_dist(tri_vbo, 1, view),
                           get_plane_dist(tri_vbo, 2, view)});
        }
      };

      index_tri* begin = reinterpret_cast<index_tri*>(tri_vbo->mIndices.data());
      index_tri* end = begin + tri_vbo->mIndices.size() / 3;

      z_dist.resize(end - begin);

      // Assumes no vertex merging, uses first vertex as a key
      for (int i = 0; i < end - begin; ++i) {
        auto first_vtx = begin[i].points[0];
        z_dist[first_vtx / 3] = begin[i].get_min_plane_dist(*tri_vbo, viewMtx);
      }

      std::sort(begin, end, [&](const index_tri& lhs, const index_tri& rhs) {
        return z_dist[lhs.points[0] / 3] > z_dist[rhs.points[0] / 3];
      });

      std::reverse(begin, end);

      tri_vbo->uploadIndexBuffer();
    }

    PushTriangles(mSceneState, glm::mat4(1.0f), viewMtx, projMtx,
                  tri_vbo->getGlId(), 3 * mKclTris.size(), disp_opts.attr_mask,
                  disp_opts.kcl_alpha);
  }

  mSceneState.buildUniformBuffers();

  librii::glhelper::ClearGlScreen();
  mSceneState.draw();

  auto pos = ImGui::GetCursorScreenPos();
  // auto win = ImGui::GetWindowPos();
  auto avail = ImGui::GetContentRegionAvail();

  // int shifted_x = mViewport.mLastWidth - avail.x;
  int shifted_y = mViewport.mLastHeight - avail.y;

  // TODO: LastWidth is moved to the right, not left -- bug?
  ImGuizmo::SetRect(pos.x, pos.y - shifted_y, mViewport.mLastWidth,
                    mViewport.mLastHeight);

  auto tVm = viewMtx;
  auto max = ImGui::GetContentRegionMaxAbs();

  const u32 viewcube_bg = 0;
  // ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_WindowBg])

  ImGuizmo::ViewManipulate(glm::value_ptr(viewMtx), 20.0f,
                           {max.x - 200.0f, max.y - 200.0f}, {200.0f, 200.0f},
                           viewcube_bg);

  auto& cam = mRenderSettings.mCameraController;
  auto cartesian = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f) * viewMtx);

#ifdef CAMERA_CONTROLLER_DEBUG
  {
    riistudio::util::ConditionalHighlight g(true);
    ImGui::Text("Cartesian: %f, %f, %f", cartesian.x, cartesian.y, cartesian.z);
  }
#endif
  if (tVm != viewMtx) {

    auto d = glm::distance(cartesian, glm::vec3(0.0f, 0.0f, 0.0f));

    cam.mHorizontalAngle = atan2f(cartesian.x, cartesian.z);
    cam.mVerticalAngle = asinf(cartesian.y / d);
  }

  static Manipulator manip;

  if (mKmp) {

    int q = 0;
    for (auto& pt : mKmp->mRespawnPoints) {
      glm::mat4 mx = librii::math::calcXform(
          {.scale =
               glm::vec3(std::max(std::min(pt.range * 50.0f, 1000.0f), 700.0f)),
           .rotation = pt.rotation,
           .translation = pt.position});
      if (q++ == 0) {

        manip.drawUi(mx);
        if (manip.manipulate(mx, viewMtx, projMtx)) {

          glm::vec3 matrixTranslation, matrixRotation, matrixScale;
          ImGuizmo::DecomposeMatrixToComponents(
              glm::value_ptr(mx), glm::value_ptr(matrixTranslation),
              glm::value_ptr(matrixRotation), glm::value_ptr(matrixScale));

          pt.position = matrixTranslation;
          pt.rotation = matrixRotation;

          assert(pt == pt);
        }
      }
    }
  }

  if (mKmp != nullptr) {
    if (!commit_posted && *mKmp != mKmpHistory[history_cursor]) {
      commit_posted = true;
    }

    if (commit_posted && !ImGui::IsAnyMouseDown()) {
      CommitHistory(history_cursor, mKmpHistory);
      mKmpHistory.push_back(*mKmp);
      assert(mKmpHistory.back() == *mKmp);
      commit_posted = false;
    }

    // TODO: Only affect active window
    if (ImGui::GetIO().KeyCtrl) {
      if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
        UndoHistory(history_cursor, mKmpHistory);
        *mKmp = mKmpHistory[history_cursor];
      } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
        RedoHistory(history_cursor, mKmpHistory);
        *mKmp = mKmpHistory[history_cursor];
      }
    }
  }
}

} // namespace riistudio::lvl
