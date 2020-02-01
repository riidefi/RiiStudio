// TODO Must be included first
#include <ThirdParty/FileDialogues.hpp>

#include "RiiCore.hpp"
#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <LibCube/Export/Exports.hpp>
#include <RiiStudio/editor/EditorWindow.hpp>
#include <RiiStudio/editor/Console.hpp>
#include <RiiStudio/api/riiapi.hpp>
void RiiCore::handleTransformAction()
{
	if (mTransformActions.empty()) return;

	auto& front = mTransformActions.front();
	ImGui::OpenPopup(front.mCfg.name.exposedName.c_str());
	if (ImGui::BeginPopupModal(front.mCfg.name.exposedName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		for (const auto& prop : front.mCfg.mParams)
		{
			switch (prop.type)
			{
			case pl::TransformStack::ParamType::String:
			{
				const auto str = front.getString(prop.name.namespacedId);
				char tmp_buf[128]{};
				memcpy_s(tmp_buf, 128, str.c_str(), str.length());
				ImGui::InputText(prop.name.exposedName.c_str(), tmp_buf, 128);
				if (std::string(tmp_buf) != str)
					front.setString(prop.name.namespacedId, tmp_buf);
				break;
			}
			case pl::TransformStack::ParamType::Flag:
			{
				bool v = front.getFlag(prop.name.namespacedId);
				bool v2 = v;
				ImGui::Checkbox(prop.name.exposedName.c_str(), &v);
				if (v != v2)
					front.setFlag(prop.name.namespacedId, v);
				break;
			}
			case pl::TransformStack::ParamType::Enum:
			{
				int e = front.getEnum(prop.name.namespacedId);
				int e2 = e;
				if (ImGui::BeginCombo(prop.name.exposedName.c_str(), prop.enumeration[e].exposedName.c_str()))
				{
					for (int i = 0; i < prop.enumeration.size(); ++i)
					{
						if (ImGui::Selectable(prop.enumeration[i].exposedName.c_str()))
						{
							e = i;
							break;
						}
					}
					ImGui::EndCombo();
				}
				if (e != e2)
					front.setEnum(prop.name.namespacedId, e);
				break;
			}
			default:
				break;
			}
		}

		if (ImGui::Button("OK"))
		{
			front.mCfg.perform(front);
			mTransformActions.pop();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			mTransformActions.pop();
		}
		ImGui::EndPopup();
	}
}


void RiiCore::drawRoot()
{
	if (mDockSpace.draw())
	{
		drawMenuBar();
	}

	handleTransformAction();

	ImGui::End();

	ImGui::Begin("WindowBar");
	if (ImGui::BeginTabBar("WindowBarWidget", 0))
	{
		for (u32 i = 0; i < getNumWindows(); ++i)
		{
			auto* wnd = (EditorWindow*)&getWindowIndexed(i);

			if (ImGui::BeginTabItem(&wnd->mFilePath.c_str()[wnd->mFilePath.rfind("\\")], &wnd->bOpen))
			{
				setActiveEditor(*wnd);
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	mConsole->draw(0);
#ifdef DEBUG
	auto ctx = makeWindowContext();
	mThemeEd.draw(&ctx);
	ImGui::ShowDemoWindow();
#endif
}
void RiiCore::save(const std::string& path)
{
	std::ofstream stream(path + "_TEST.bmd", std::ios::binary | std::ios::out);

	oishii::v2::Writer writer(1024);

	EditorWindow* ed = (EditorWindow*)getActiveEditor();
	if (!ed) return;

	auto ex = mPluginFactory.spawnExporter(ed->mState->mName.namespacedId);
	ex->write(writer, *ed->mState.get());

	stream.write((const char*)writer.getDataBlockStart(), writer.getBufSize());
}
void RiiCore::saveAs()
{
	auto results = pfd::save_file("Save File", "", { "All Files", "*" }).result();
	if (results.empty())
		return;

	save(results);
}

void RiiCore::drawMenuBar()
{
	EditorWindow* ed = (EditorWindow*)getActiveEditor();
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
					DebugReport("Attempting to save to %s\n", ed->mFilePath.c_str());
					if (!ed->mFilePath.empty())
						save(ed->mFilePath);
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
			if (ed && ed->mWindowCollection)
			{
				for (int i = 0; i < ed->mWindowCollection->getNum(); ++i)
				{
					if (ImGui::MenuItem(ed->mWindowCollection->getName(i)))
					{
						ed->attachWindow(ed->mWindowCollection->spawn(i), ed->mId);
					}
				}
			}
			ImGui::End();
		}

		if (ed)
			drawTransformsMenu(*ed);
		ImGui::EndMenuBar(); // Placement?
	}
}

void RiiCore::recursiveTransformsMenuItem(const std::string& type, EditorWindow& window)
{
	std::vector<void*> out;
	mPluginFactory.findParentOfType(out, window.mState.get(), window.mState->mName.namespacedId, "transform_stack");

	for (void* it_raw : out)
	{
		pl::TransformStack* it = reinterpret_cast<pl::TransformStack*>(it_raw);
		assert(it);
		if (!it)
			return;
		for (auto& xf : it->mStack)
			if (ImGui::MenuItem(xf->name.exposedName.c_str()))
				mTransformActions.push(*xf);
	}
}

void RiiCore::drawTransformsMenu(EditorWindow& window)
{
	if (ImGui::BeginMenu("Transform"))
	{
		recursiveTransformsMenuItem(window.mState->mName.namespacedId, window);
		ImGui::EndMenu();
	}
}

std::vector<std::string> RiiCore::fileDialogueOpen()
{
	return pfd::open_file("Open File").result();	
}
void RiiCore::openFile(const std::string& file, OpenFilePolicy policy)
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

		auto importer = mPluginFactory.spawnImporter(file, *reader.get());

		if (!importer.has_value())
			return;


		auto fileState = mPluginFactory.spawnFileState(importer->fileStateId);
		if (fileState.get() == nullptr)
			return;

		mCoreRes.numPluginWindow++;

		importer->importer->tryRead(*reader.get(), *fileState.get());

		auto edWindow = std::make_unique<EditorWindow>(std::move(fileState), mPluginFactory, file);

		if (policy == OpenFilePolicy::NewEditor)
		{
			attachWindow(std::move(edWindow));
		}
		else if (policy == OpenFilePolicy::ReplaceEditor)
		{
			if (getActiveEditor())
				detachWindow(getActiveEditor()->mId);
			attachWindow(std::move(edWindow));
		}
		else if (policy == OpenFilePolicy::ReplaceEditorIfMatching)
		{
			if (getActiveEditor() && ((EditorWindow*)getActiveEditor())->mState->mName.namespacedId == fileState->mName.namespacedId)
			{
				detachWindow(getActiveEditor()->mId);	
			}

			attachWindow(std::move(edWindow));
		}
		else
		{
			throw "Invalid open file policy";
		}
	}
}
void RiiCore::openFile(OpenFilePolicy policy)
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

static RiiCore* spCore = nullptr;

static void core_drop_file(const char* path)
{
	printf("Dropping file: %s\n", path);

	if (spCore)
		spCore->openFile(path, RiiCore::OpenFilePolicy::NewEditor);
}

static void core_drop(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; ++i)
		core_drop_file(paths[i]);
}

RiiCore::RiiCore()
	: Applet("RiiStudio")
{
	mPluginFactory.registerPlugin(libcube::Package(mPluginFactory));
	mPluginFactory.installModule("nw.dll");
	mPluginFactory.computeDataMesh();

	assert(!spCore);
	spCore = this;
	setDropCallback(core_drop);

	// bOnlyDrawActive = true;

	mTheme.mThemeSelection = ThemeManager::BasicTheme::Raikiri;
	mTheme.mThemeManager.setThemeEx(mTheme.mThemeSelection);

	mConsole = std::make_unique<Console>(*this);
	ConsoleHandle::sCH = new ConsoleHandle(*(mConsole.get()));
}
RiiCore::~RiiCore()
{
	delete ConsoleHandle::sCH;
}
