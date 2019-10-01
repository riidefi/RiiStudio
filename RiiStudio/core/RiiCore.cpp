#include "RiiCore.hpp"
#include <FileDialogues.hpp>
#include <fstream>
#include <oishii/reader/binary_reader.hxx>

void RiiCore::drawRoot()
{
	mDockSpace.draw();
	{
		drawMenuBar();
	}
	ImGui::End();
}

void RiiCore::drawMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open"))
			{
				auto toOpen = fileDialogueOpen();
				for (const auto& str : toOpen)
					DebugReport("%s\n", str.c_str());
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar(); // Placement?
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

		// TODO: Implement IReadable in plugins and call.
	}
}
