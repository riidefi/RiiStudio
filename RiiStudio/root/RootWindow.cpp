#include "RootWindow.hpp"

#include <ThirdParty/FileDialogues.hpp>
#include <fstream>
#include <oishii/reader/binary_reader.hxx>

#include <LibCore/api/impl/api.hpp>

#include <LibCube/Export/Install.hpp>
#include <LibJ/Install.hpp>

#include <LibCore/api/Node.hpp>

#include <RiiStudio/editor/EditorWindow.hpp>

void RootWindow::draw(Window* ctx) noexcept
{
	ImGui::PushID(0);

	while (!mDropQueue.empty())
	{
		openFile(mDropQueue.back(), OpenFilePolicy::NewEditor);
		mDropQueue.pop();
	}

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

	ImGuiID dockspace_id = ImGui::GetID("DockSpaceWidget");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);

	EditorWindow* ed = dynamic_cast<EditorWindow*>(getActiveWindow());

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
				
				if (ed)
				{
					DebugReport("Attempting to save to %s\n", ed->getFilePath().c_str());
					if (!ed->getFilePath().empty())
						save(ed->getFilePath());
					else
						saveAs();
				}
				else
				{
					printf("Cannot save.. nothing has been opened.\n");
				}
			}
			if (ImGui::MenuItem("Save As"))
			{
				if (ed)
					saveAs();
				else printf("Cannot save.. nothing has been opened.\n");
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
			ImGui::End();
		}


		ImGui::EndMenuBar();
	}


    // ImGui::ShowDemoWindow();

	drawChildren();


	// Handle popups
	ImGui::End();
	ImGui::PopID();
}

void RootWindow::save(const std::string& path)
{
	std::ofstream stream(path + "_TEST.bmd", std::ios::binary | std::ios::out);

	oishii::v2::Writer writer(1024);

	EditorWindow* ed = dynamic_cast<EditorWindow*>(getActiveWindow());
	if (!ed) return;

	auto ex = SpawnExporter(ed->getState().mType);
	if (!ex) DebugReport("Failed to spawn importer.\n");
	if (!ex) return;
	ex->write(ed->getState(), writer);

	stream.write((const char*)writer.getDataBlockStart(), writer.getBufSize());
}
void RootWindow::saveAs()
{
	auto results = pfd::save_file("Save File", "", { "All Files", "*" }).result();
	if (results.empty())
		return;

	save(results);
}

std::vector<std::string> RootWindow::fileDialogueOpen()
{
	return pfd::open_file("Open File").result();	
}

void RootWindow::openFile(const std::string& file, OpenFilePolicy policy)
{
	std::ifstream stream(file, std::ios::binary | std::ios::ate);

	if (!stream)
		return;

	const auto size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	std::vector<u8> data(size);
	if (stream.read(reinterpret_cast<char*>(data.data()), size))
	{
		auto reader = std::make_unique<oishii::BinaryReader>(std::move(data), size, file.c_str());

		auto importer = SpawnImporter(file, *reader.get());
		
		if (!importer.second)
			return;


		auto fileState = SpawnState(importer.first);
		if (fileState.mBase == nullptr)
			return;

		importer.second->read(fileState, *reader.get());

		auto edWindow = std::make_unique<EditorWindow>(std::move(fileState), file);

		if (policy == OpenFilePolicy::NewEditor)
		{
			attachWindow(std::move(edWindow));
		}
		else if (policy == OpenFilePolicy::ReplaceEditor)
		{
			//	if (getActiveEditor())
			//		detachWindow(getActiveEditor()->mId);
			attachWindow(std::move(edWindow));
		}
		else if (policy == OpenFilePolicy::ReplaceEditorIfMatching)
		{
			//	if (getActiveEditor() && ((EditorWindow*)getActiveEditor())->mState->mName.namespacedId == fileState->mName.namespacedId)
			//	{
			//		detachWindow(getActiveEditor()->mId);	
			//	}

			attachWindow(std::move(edWindow));
		}
		else
		{
			throw "Invalid open file policy";
		}
	}
}
void RootWindow::openFile(OpenFilePolicy policy)
{
	auto results = fileDialogueOpen();

	if (results.empty())
		return;

	assert(results.size() == 1);
	if (results.size() != 1)
		return;

	auto file = results[0];
	openFile(file, policy);
}

void RootWindow::drop(const std::vector<std::string_view>& paths)
{
	for (const auto& path : paths)
	{
		DebugReport("Dropping file: %s\n", path.data());
		mDropQueue.push(std::string(path));
	}
}

RootWindow::RootWindow()
    : Applet("RiiStudio")
{
	InitAPI();

    // Register plugins
	libcube::Install();
	libcube::jsystem::Install();

	px::ReflectionMesh::getInstance()->getDataMesh().compute();
	
    // Theme defaults
	mTheme.setTheme<ThemeManager::BasicTheme::ZenithDark>();
	
}
RootWindow::~RootWindow()
{
	DeinitAPI();
}
