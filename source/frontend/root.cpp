#include "root.hpp"

#include <vendor/imgui/imgui.h>
#include <vendor/imgui/imgui_internal.h>

#include <frontend/editor/editor_window.hpp>

#include <core/api.hpp>

#include <oishii/reader/binary_reader.hxx>

#include <plugins/gc/Export/Install.hpp>

namespace riistudio::g3d {
void Install();
}

namespace riistudio::frontend {

void RootWindow::draw() {
	fileHostProcess();

    ImGui::PushID(0);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace##", &bOpen, window_flags);
	ImGui::PopStyleVar(3);

	dockspace_id = ImGui::GetID("DockSpaceWidget");

	ImGuiID dock_main_id = dockspace_id;
	while (mAttachEditorsQueue.size())
	{
		const std::string& ed_id = mAttachEditorsQueue.front();

		ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
		ImGui::DockBuilderAddNode(dockspace_id); // Add empty node
		
		
		ImGuiID dock_up_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.05f, nullptr, &dock_main_id);
		ImGui::DockBuilderDockWindow(ed_id.c_str(), dock_up_id);
		

		ImGui::DockBuilderFinish(dockspace_id);
		mAttachEditorsQueue.pop();
	}

	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);

	EditorWindow* ed = dynamic_cast<EditorWindow*>(mActive);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open"))
			{
				openFile();
			}
			if (ImGui::MenuItem("Save"))
			{
				
				//  if (ed)
				//  {
				//  	DebugReport("Attempting to save to %s\n", ed->getFilePath().c_str());
				//  	if (!ed->getFilePath().empty())
				//  		save(ed->getFilePath());
				//  	else
				//  		saveAs();
				//  }
				//  else
				{
					printf("Cannot save.. nothing has been opened.\n");
				}
			}
			if (ImGui::MenuItem("Save As"))
			{
				//  if (ed)
				//  	saveAs();
				// else
                    printf("Cannot save.. nothing has been opened.\n");
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Windows"))
		{
			//	if (ed && ed->mWindowCollection)
			//	{
			//		for (int i = 0; i < ed->mWindowCollection->getNum(); ++i)
			//		{
			//			if (ImGui::MenuItem(ed->mWindowCollection->getName(i)))
			//			{
			//				ed->attachWindow(ed->mWindowCollection->spawn(i), ed->mId);
			//			}
			//		}
			//	}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			bool _vsync = vsync;
			ImGui::Checkbox("VSync", &_vsync);

			if (_vsync != vsync)
			{
				setVsync(_vsync);
				vsync = _vsync;
			}

			ImGui::EndMenu();
		}

		const auto& io = ImGui::GetIO();
		ImGui::SameLine(ImGui::GetWindowWidth() - 60);
#ifndef NDEBUG
		ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
#else
		ImGui::Text("%u fps", static_cast<u32>(roundf(io.Framerate)));
#endif

		ImGui::EndMenuBar();
	}


    // ImGui::ShowDemoWindow();

	drawChildren();


	// Handle popups
	ImGui::End();
	ImGui::PopID();
}

std::vector<std::string> GetChildrenOfType(const std::string& type)
{
	std::vector<std::string> out;
	const auto hnd = kpi::ReflectionMesh::getInstance()->lookupInfo(type);

	for (int i = 0; i < hnd.getNumChildren(); ++i)
	{
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
	auto reader = std::make_unique<oishii::BinaryReader>(std::move(vdata), (u32)data.mLen, data.mPath.c_str());
	auto importer = SpawnImporter(data.mPath, *reader.get());

	if (!importer.second) {
		printf("Cannot spawn importer..\n");
		return;
	}
	if (!IsConstructible(importer.first))
	{
		printf("Non constructable state.. find parents\n");

		const auto children = GetChildrenOfType(importer.first);
		if (children.empty())
		{
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

	auto edWindow = std::make_unique<EditorWindow>(std::move(fileState), reader->getFile());

	if (policy == OpenFilePolicy::NewEditor)
	{
		attachEditorWindow(std::move(edWindow));
	}
	else if (policy == OpenFilePolicy::ReplaceEditor)
	{
		//	if (getActiveEditor())
		//		detachWindow(getActiveEditor()->mId);
		attachEditorWindow(std::move(edWindow));
	}
	else if (policy == OpenFilePolicy::ReplaceEditorIfMatching)
	{
		//	if (getActiveEditor() && ((EditorWindow*)getActiveEditor())->mState->mName.namespacedId == fileState->mName.namespacedId)
		//	{
		//		detachWindow(getActiveEditor()->mId);	
		//	}

		attachEditorWindow(std::move(edWindow));
	}
	else
	{
		throw "Invalid open file policy";
	}
}
void RootWindow::attachEditorWindow(std::unique_ptr<EditorWindow> editor)
{
	mAttachEditorsQueue.push(editor->getName());

	mChildren.push_back(std::move(editor));
}

RootWindow::RootWindow()
	: Applet("RiiStudio")
{
	InitAPI();

	// Register plugins
	//	lib3d::install();
	libcube::Install();
	// libcube::jsystem::Install();
	g3d::Install();

	//	px::PackageInstaller::spInstance->installModule("nw.dll");

	kpi::ReflectionMesh::getInstance()->getDataMesh().compute();

	// Theme defaults
	// mTheme.setTheme<ThemeManager::BasicTheme::ZenithDark>();
}
RootWindow::~RootWindow()
{
	DeinitAPI();
}
}
