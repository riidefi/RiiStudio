// TODO Must be included first
#include <ThirdParty/FileDialogues.hpp>

#include "RiiCore.hpp"
#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <LibCube/Export/Exports.hpp>
#include <RiiStudio/editor/EditorWindow.hpp>

void RiiCore::drawRoot()
{
	if (mDockSpace.draw())
	{
		drawMenuBar();
	}
	if (!mTransformActions.empty())
	{
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

	ImGui::End();
#ifdef DEBUG
	auto ctx = makeWindowContext();
	mThemeEd.draw(&ctx);
	ImGui::ShowDemoWindow();
#endif
}
#if 0
// DEBUG
void RiiCore::DEBUGwriteBmd(const std::string& results)
{
	std::ofstream stream(results, std::ios::binary | std::ios::out);

	if (stream)
	{
		auto buf = std::make_unique<std::vector<u8>>();
		buf->reserve(1024 * 1024);
		oishii::Writer writer(std::move(buf), 0);

		EditorWindow* ed = getActiveEditor();
		if (!ed) return;
		auto ex = mPluginFactory.spawnExporter("j3dcollection");
		ex->write(writer, *ed->mState.get());

		stream.write((const char*)writer.getDataBlockStart(), writer.getBufSize());
	}
}
#endif
void RiiCore::save(const std::string& path)
{
	std::ofstream stream(path + "_TEST.bmd", std::ios::binary | std::ios::out);

	auto buf = std::make_unique<std::vector<u8>>();
	buf->reserve(1024 * 1024);
	oishii::Writer writer(std::move(buf), 0);

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
#if 0
			// TODO -- Just for debugging
			if (ImGui::MenuItem("Save BMD"))
			{
#if 0
				auto results = pfd::save_file("Save File", "", { "All Files", "*", "J3D Binary Model Data", "*.bmd", "J3D Binary Display List", "*.bdl" }).result();
				if (!results.empty())
				{
					DEBUGwriteBmd(results);
				}
#endif
				DEBUGwriteBmd("debug_out.bmd");
			}
#endif
			ImGui::EndMenu();
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

void RiiCore::openFile(OpenFilePolicy policy)
{
	// TODO: Support other policies
	if (policy != OpenFilePolicy::NewEditor)
		return;

	auto results = fileDialogueOpen();

	if (results.empty())
		return;

	assert(results.size() == 1);
	if (results.size() != 1)
		return;

	auto file = results[0];

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

		if (policy == OpenFilePolicy::NewEditor)
		{
			auto fileState = mPluginFactory.spawnFileState(importer->fileStateId);
			if (fileState.get() == nullptr)
				return;

			mCoreRes.numPluginWindow++;

			importer->importer->tryRead(*reader.get(), *fileState.get());

			auto edWindow = std::make_unique<EditorWindow>(std::move(fileState), mPluginFactory, file);
			
			attachWindow(std::move(edWindow));
			
		}

		// TODO -- Check filestate id against current
		// TODO -- Invoke spawned importer
	}
}


RiiCore::RiiCore()
{
	mPluginFactory.registerPlugin(libcube::Package());
	mPluginFactory.installModule("nw.dll");
	mPluginFactory.computeDataMesh();
}
RiiCore::~RiiCore()
{}
