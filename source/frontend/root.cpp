#include <vendor/pfd/portable-file-dialogs.h>

#include "root.hpp"

#include <vendor/imgui/imgui.h>
#include <vendor/imgui/imgui_internal.h>

#include <frontend/editor/editor_window.hpp>

#include <core/api.hpp>

#include <oishii/reader/binary_reader.hxx>

#include <plugins/gc/Export/Install.hpp>
#include <plugins/j3d/Installer.hpp>
#include <plugins/pik/installer.hpp>

#include <fstream>
#include <oishii/v2/writer/binary_writer.hxx>

// Experimental conversion
#include <plugins/j3d/Scene.hpp>

namespace riistudio::g3d {
void Install();
}

namespace riistudio::frontend {

void RootWindow::draw() {
  fileHostProcess();

  ImGui::PushID(0);

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace##Root", &bOpen, window_flags);
  ImGui::PopStyleVar(3);

  dockspace_id = ImGui::GetID("DockSpaceWidget");

  ImGuiID dock_main_id = dockspace_id;
  while (mAttachEditorsQueue.size()) {
    const std::string& ed_id = mAttachEditorsQueue.front();

    // ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
    // ImGui::DockBuilderAddNode(dockspace_id); // Add empty node
    //
    //
    // ImGuiID dock_up_id = ImGui::DockBuilderSplitNode(dock_main_id,
    // ImGuiDir_Up, 0.05f, nullptr, &dock_main_id);
    // ImGui::DockBuilderDockWindow(ed_id.c_str(), dock_up_id);
    //
    //
    // ImGui::DockBuilderFinish(dockspace_id);
    mAttachEditorsQueue.pop();
  }

  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);

  EditorWindow* ed = dynamic_cast<EditorWindow*>(
      mActive ? mActive : !mChildren.empty() ? mChildren[0].get() : this);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open")) {
        openFile();
      }
      if (ImGui::MenuItem("Save")) {

        if (ed) {
          DebugReport("Attempting to save to %s\n", ed->getFilePath().c_str());
          if (!ed->getFilePath().empty())
            save(ed->getFilePath());
          else
            saveAs();
        } else {
          printf("Cannot save.. nothing has been opened.\n");
        }
      }
      if (ImGui::MenuItem("Save As")) {
        if (ed)
          saveAs();
        else
          printf("Cannot save.. nothing has been opened.\n");
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Windows")) {
      //	if (ed && ed->mWindowCollection)
      //	{
      //		for (int i = 0; i < ed->mWindowCollection->getNum();
      //++i)
      //		{
      //			if
      //(ImGui::MenuItem(ed->mWindowCollection->getName(i)))
      //			{
      //				ed->attachWindow(ed->mWindowCollection->spawn(i),
      // ed->mId);
      //			}
      //		}
      //	}
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Settings")) {
      bool _vsync = vsync;
      ImGui::Checkbox("VSync", &_vsync);

      if (_vsync != vsync) {
        setVsync(_vsync);
        vsync = _vsync;
      }

      ImGui::Checkbox("Theme Editor", &bThemeEditor);

      ImGui::EndMenu();
    }
#ifndef BUILD_DIST
    if (ImGui::BeginMenu("Experimental")) {
      if (ImGui::MenuItem("Convert to BMD") && ed != nullptr) {
        const kpi::IDocumentNode* from_root = ed->mState.get();
        auto* from_models = from_root->getFolder<lib3d::Model>();
        auto* from_textures = from_root->getFolder<libcube::Texture>();

        auto bmd_state = SpawnState(typeid(j3d::Collection).name());

        j3d::CollectionAccessor bmd_col(bmd_state.get());

        for (int fr_i = 0; fr_i < from_models->size(); ++fr_i) {
          const auto& from_model = from_models->at<kpi::IDocumentNode>(fr_i);
          auto bmd_model = bmd_col.addModel();
          // info
          // Bufs (automate)
          bmd_model.get().mBufs.norm.mQuant.comp.normal =
              libcube::gx::VertexComponentCount::Normal::xyz;
          // drawmtx (skip)
          auto& mtx = bmd_model.get().mDrawMatrices.emplace_back();
          mtx.mWeights.emplace_back(0, 1.0f);
          // materials
          {
            const auto from_mats = from_model.getFolder<libcube::IGCMaterial>();
            for (int m_i = 0; m_i < from_mats->size(); ++m_i) {
              auto mat = bmd_model.addMaterial();
              mat.get().id = m_i;

              auto& md = mat.get().getMaterialData();

              mat.get().getMaterialData() =
                  from_mats->at<libcube::IGCMaterial>(m_i).getMaterialData();

              auto before = mat.get().getMaterialData().samplers;
              md.samplers = {};
              for (int i = 0; i < before.size(); ++i) {
                auto simp =
                    std::make_unique<j3d::MaterialData::J3DSamplerData>();
                static_cast<libcube::GCMaterialData::SamplerData&>(
                    *simp.get()) =
                    static_cast<const libcube::GCMaterialData::SamplerData&>(
                        *before[i].get());
                md.samplers.push_back(std::move(simp));
              }
              mat.get().indEnabled =
                  mat.get().getMaterialData().info.nIndStage > 0;
            }
          }
          // joints
          {
            auto from_joints = from_model.getFolder<libcube::IBoneDelegate>();
            for (int m_i = 0; m_i < from_joints->size(); ++m_i) {
              auto joint = bmd_model.addJoint();
              joint.get().id = m_i;
              from_joints->at<libcube::IBoneDelegate>(m_i).copy(joint.get());
            }
          }
          // shapes
          {
            auto from_shapes = from_model.getFolder<libcube::IndexedPolygon>();
            for (int m_i = 0; m_i < from_shapes->size(); ++m_i) {
              auto& from_shape = from_shapes->at<libcube::IndexedPolygon>(m_i);
              const auto vcd = from_shape.getVcd();

              auto& bmd_shape = bmd_model.addShapeRaw();
              bmd_shape.id = m_i;
              bmd_shape.mVertexDescriptor = vcd;
              for (auto& e : bmd_shape.mVertexDescriptor.mAttributes) {
                e.second = libcube::gx::VertexAttributeType::Short;
              }
              bmd_shape.mVertexDescriptor
                  .calcVertexDescriptorFromAttributeList();

              for (int i = 0; i < from_shape.getNumMatrixPrimitives(); ++i) {
                auto& bmd_mp = bmd_shape.mMatrixPrimitives.emplace_back();
                bmd_mp.mCurrentMatrix = 0;
                bmd_mp.mDrawMatrixIndices.push_back(0);
                // No multi mtx yet
                for (int j = 0;
                     j < from_shape.getMatrixPrimitiveNumIndexedPrimitive(i);
                     ++j) {
                  const auto& prim =
                      from_shape.getMatrixPrimitiveIndexedPrimitive(i, j);
                  auto& p = bmd_mp.mPrimitives.emplace_back(prim);
                  // Remap vtx indices
                  for (auto& v : p.mVertices) {
                    for (u32 x = 0; x < (u32)libcube::gx::VertexAttribute::Max;
                         ++x) {
                      if (!(vcd.mBitfield & (1 << x)))
                        continue;
                      auto& bufs = bmd_model.get().mBufs;
                      switch (static_cast<libcube::gx::VertexAttribute>(x)) {
                      case libcube::gx::VertexAttribute::
                          PositionNormalMatrixIndex:
                      case libcube::gx::VertexAttribute::Texture0MatrixIndex:
                      case libcube::gx::VertexAttribute::Texture1MatrixIndex:
                      case libcube::gx::VertexAttribute::Texture2MatrixIndex:
                      case libcube::gx::VertexAttribute::Texture3MatrixIndex:
                      case libcube::gx::VertexAttribute::Texture4MatrixIndex:
                      case libcube::gx::VertexAttribute::Texture5MatrixIndex:
                      case libcube::gx::VertexAttribute::Texture6MatrixIndex:
                      case libcube::gx::VertexAttribute::Texture7MatrixIndex:
                        break;
                      case libcube::gx::VertexAttribute::Position: {
                        auto pos = from_shape.getPos(
                            v[libcube::gx::VertexAttribute::Position]);
                        auto found = std::find(bufs.pos.mData.begin(),
                                               bufs.pos.mData.end(), pos);
                        if (found == bufs.pos.mData.end()) {
                          bmd_model.get().mBufs.pos.mData.push_back(pos);
                          v[libcube::gx::VertexAttribute::Position] =
                              bmd_model.get().mBufs.pos.mData.size() - 1;
                        } else {
                          v[libcube::gx::VertexAttribute::Position] =
                              found - bufs.pos.mData.begin();
                        }
                        break;
                      }
                      case libcube::gx::VertexAttribute::Color0: {
                        const auto scolor = from_shape.getClr(
                            v[libcube::gx::VertexAttribute::Color0]);
                        libcube::gx::Color clr;
                        clr.r = roundf(scolor[0] * 255.0f);
                        clr.g = roundf(scolor[1] * 255.0f);
                        clr.b = roundf(scolor[2] * 255.0f);
                        clr.a = roundf(scolor[3] * 255.0f);
                        bmd_model.get().mBufs.color[0].mData.push_back(clr);
                        v[libcube::gx::VertexAttribute::Color0] =
                            bmd_model.get().mBufs.color[0].mData.size() - 1;
                        break;
                      }
                      case libcube::gx::VertexAttribute::TexCoord0:
                      case libcube::gx::VertexAttribute::TexCoord1:
                      case libcube::gx::VertexAttribute::TexCoord2:
                      case libcube::gx::VertexAttribute::TexCoord3:
                      case libcube::gx::VertexAttribute::TexCoord4:
                      case libcube::gx::VertexAttribute::TexCoord5:
                      case libcube::gx::VertexAttribute::TexCoord6:
                      case libcube::gx::VertexAttribute::TexCoord7: {
                        const auto chan =
                            x - static_cast<int>(
                                    libcube::gx::VertexAttribute::TexCoord0);
                        const auto attr =
                            static_cast<libcube::gx::VertexAttribute>(x);
                        const auto data = from_shape.getUv(chan, v[attr]);

                        bmd_model.get().mBufs.uv[chan].mData.push_back(data);
                        v[static_cast<libcube::gx::VertexAttribute>(x)] =
                            bmd_model.get().mBufs.uv[chan].mData.size() - 1;

                        break;
                      }
                      case libcube::gx::VertexAttribute::Normal:
                        bmd_model.get().mBufs.norm.mData.push_back(
                            from_shape.getNrm(
                                v[libcube::gx::VertexAttribute::Normal]));
                        v[libcube::gx::VertexAttribute::Normal] =
                            bmd_model.get().mBufs.norm.mData.size() - 1;
                        break;
                      default:
                        throw "Invalid vtx attrib";
                        break;
                      }
                    }
                  }
                }
              }
            }
          }
        }
        // textures
        for (int fr_i = 0; fr_i < from_textures->size(); ++fr_i) {
          const auto& from_texture = from_textures->at<libcube::Texture>(fr_i);
          auto bmd_texture = bmd_col.addTexture();

          auto& bt = bmd_texture.get();
          bt.mName = from_texture.getName();
          bt.mFormat = from_texture.getTextureFormat();
          bt.bTransparent = false; // TODO
          bt.mWidth = from_texture.getWidth();
          bt.mHeight = from_texture.getHeight();
          bt.mPaletteFormat = from_texture.getPaletteFormat();
          bt.nPalette = 0;
          bt.ofsPalette = 0;
          bt.mMinLod = 0; // TODO: Verify
          bt.mMaxLod = from_texture.getMipmapCount() + 1;
          bt.mMipmapLevel = from_texture.getMipmapCount() + 1;
          bt.mData.resize(from_texture.getEncodedSize(true));
          memcpy(bt.mData.data(), from_texture.getData(), bt.mData.size());
          // TODO: Copy
          // bmd_texture.get()
        }

        attachEditorWindow(
            std::make_unique<EditorWindow>(std::move(bmd_state), "__conv.bmd"));
      }
      ImGui::EndMenu();
    }
#endif

    const auto& io = ImGui::GetIO();
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
#ifndef NDEBUG
    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate,
                io.Framerate ? 1000.0f / io.Framerate : 0.0f);
#else
    ImGui::Text("%u fps", static_cast<u32>(roundf(io.Framerate)));
#endif

    ImGui::EndMenuBar();
  }

  drawChildren();

  if (bThemeEditor && ImGui::Begin("Theme Editor", &bThemeEditor)) {
    int sel = static_cast<int>(mCurTheme);
    ImGui::Combo("Theme", &sel, ThemeManager::ThemeNames);
    mCurTheme = static_cast<ThemeManager::BasicTheme>(sel);
    ImGui::SliderFloat("Font Scale", &mFontGlobalScale, 0.1f, 2.0f);
    ImGui::End();
  }
  mTheme.setThemeEx(mCurTheme);

  ImGui::GetIO().FontGlobalScale = mFontGlobalScale;

  // Handle popups
  ImGui::End();
  ImGui::PopID();
}

std::vector<std::string> GetChildrenOfType(const std::string& type) {
  std::vector<std::string> out;
  const auto hnd = kpi::ReflectionMesh::getInstance()->lookupInfo(type);

  for (int i = 0; i < hnd.getNumChildren(); ++i) {
    out.push_back(hnd.getChild(i).getName());
    for (const auto& str : GetChildrenOfType(hnd.getChild(i).getName()))
      out.push_back(str);
  }
  return out;
}
void RootWindow::onFileOpen(FileData data, OpenFilePolicy policy) {
  printf("Opening file: %s\n", data.mPath.c_str());

  std::vector<u8> vdata(data.mLen);
  memcpy(vdata.data(), data.mData.get(), data.mLen);
  auto reader = std::make_unique<oishii::BinaryReader>(
      std::move(vdata), (u32)data.mLen, data.mPath.c_str());
  auto importer = SpawnImporter(data.mPath, *reader.get());

  if (!importer.second) {
    printf("Cannot spawn importer..\n");
    return;
  }
  if (!IsConstructible(importer.first)) {
    printf("Non constructable state.. find parents\n");

    const auto children = GetChildrenOfType(importer.first);
    if (children.empty()) {
      printf("No children. Cannot construct.\n");
      return;
    }
    assert(children.size() == 1 && IsConstructible(children[0])); // TODO
    importer.first = children[0];
  }

  auto fileState = SpawnState(importer.first);
  if (!fileState.get()) {
    printf("Cannot spawn file state %s.\n", importer.first.c_str());
    return;
  }

  importer.second->read_(*fileState.get(), *reader.get());

  auto edWindow =
      std::make_unique<EditorWindow>(std::move(fileState), reader->getFile());

  if (policy == OpenFilePolicy::NewEditor) {
    attachEditorWindow(std::move(edWindow));
  } else if (policy == OpenFilePolicy::ReplaceEditor) {
    //	if (getActiveEditor())
    //		detachWindow(getActiveEditor()->mId);
    attachEditorWindow(std::move(edWindow));
  } else if (policy == OpenFilePolicy::ReplaceEditorIfMatching) {
    //	if (getActiveEditor() &&
    //((EditorWindow*)getActiveEditor())->mState->mName.namespacedId ==
    // fileState->mName.namespacedId)
    //	{
    //		detachWindow(getActiveEditor()->mId);
    //	}

    attachEditorWindow(std::move(edWindow));
  } else {
    throw "Invalid open file policy";
  }
}
void RootWindow::attachEditorWindow(std::unique_ptr<EditorWindow> editor) {
  mAttachEditorsQueue.push(editor->getName());
#ifdef RII_BACKEND_GLFW
  editor->mpGlfwWindow = mpGlfwWindow;
  for (auto& child : editor->mChildren) {
    child->mpGlfwWindow = mpGlfwWindow;
  }
#endif
  mChildren.push_back(std::move(editor));
}

RootWindow::RootWindow() : Applet("RiiStudio") {
  InitAPI();

  // Register plugins
  //	lib3d::install();
  libcube::Install();
  j3d::Install(*kpi::ApplicationPlugins::getInstance());
  g3d::Install();
  pik::Install(*kpi::ApplicationPlugins::getInstance());

  //	px::PackageInstaller::spInstance->installModule("nw.dll");

  kpi::ReflectionMesh::getInstance()->getDataMesh().compute();
}
RootWindow::~RootWindow() { DeinitAPI(); }

void RootWindow::save(const std::string& path) {
  std::ofstream stream(path + "_TEST.bmd", std::ios::binary | std::ios::out);

  oishii::v2::Writer writer(1024);

  EditorWindow* ed = dynamic_cast<EditorWindow*>(
      mActive ? mActive : !mChildren.empty() ? mChildren[0].get() : this);
  if (!ed)
    return;

  auto ex = SpawnExporter(*ed->mState.get());
  if (!ex)
    DebugReport("Failed to spawn importer.\n");
  if (!ex)
    return;
  ex->write_(*ed->mState.get(), writer);

  stream.write((const char*)writer.getDataBlockStart(), writer.getBufSize());
}
void RootWindow::saveAs() {
  auto results = pfd::save_file("Save File", "", {"All Files", "*"}).result();
  if (results.empty())
    return;

  save(results);
}

} // namespace riistudio::frontend
