#include "MatEditor.hpp"
#include <array>
#include <ThirdParty/imgui/imgui.h>
#include "Components/CullMode.hpp"
#include <LibRiiEditor/core/SelectionManager.hpp>
namespace ed {

enum class MatTab
{
	DisplaySurface,
	Color,

#ifdef DEBUG
	Debug,
#endif
	Max
};
static const std::array<const char*, static_cast<size_t>(MatTab::Max)> MatTabNames = {
	"Surface Visibility",
	"Color",
#ifdef DEBUG
	"DEBUG"
#endif
};

template<void(*PredFunc)(void)>
struct Predicate
{
	~Predicate() { PredFunc(); }
};

void MaterialEditor::draw(WindowContext* ctx) noexcept
{
	Predicate<ImGui::End> g;
	ImGuiWindowClass cl;
	cl.ClassId = parentId;
	ImGui::SetNextWindowClass(&cl);
	if (!ImGui::Begin("MatEditor", &bOpen) || !ctx)
		return;
		
	std::vector<libcube::IMaterialDelegate*> mats;

	for (const auto& sel : ctx->selectionManager.mSelections)
	{
		if (sel.type == SelectionManager::Type::Material)
		{
			mats.push_back(static_cast<libcube::IMaterialDelegate*>(sel.object));
		}
	}

	if (mats.empty())
	{
		ImGui::Text("No material has been loaded.");
		return;
	}
	// Note: Active changed from back->front
	ImGui::Text("%s %s (%u)", mats.front()->getName().c_str(), mats.size() > 1 ? "..." : "", mats.size());

	if (drawLeft(mats) && selected != -1 && selected < (int)MatTab::Max)
	{
		ImGui::SameLine();
		ImGui::BeginGroup();
		drawRight(mats, *mats.front());
		ImGui::EndGroup();
	}
}

bool MaterialEditor::drawLeft(std::vector<libcube::IMaterialDelegate*>& mats)
{
	Predicate<ImGui::EndChild> g;

	if (!ImGui::BeginChild("left pane", ImVec2(150, 0), true))
		return false;
	
	for (int i = 0; i < (int)MatTab::Max; i++)
	{
		if (ImGui::Selectable(MatTabNames[i], selected == i))
			selected = i;
	}
	return true;
}
using m = libcube::IMaterialDelegate;
using p = m::PropertySupport;

bool MaterialEditor::drawGenInfoTab(std::vector<MatDelegate*>& mats, MatDelegate& active_selection)
{
	auto drawCullMode = [&]() {
		if (!active_selection.support.canRead(p::Feature::CullMode))
			return;

		ed::CullMode d(active_selection.getCullMode());
		d.draw();

		if (!active_selection.support.canWrite(p::Feature::CullMode) || d.get() == active_selection.getCullMode())
			return;

		for (auto p : mats)
			p->setCullMode(d.get());
	};
	
	drawCullMode();

	if (active_selection.support.canRead(p::Feature::GenInfo))
	{
		const auto gen = active_selection.getGenInfo();

		ImGui::Separator();
		ImGui::Text("Gen Info");
		ImGui::Text("Number of XF Color Channels: %u", gen.colorChan);
		ImGui::Text("Number of XF Texture Coordinate Generators: %u", gen.texGen);
		ImGui::Text("Number of TEV Pixel Shader Stages: %u", gen.tevStage);
		ImGui::Text("Number of BUMP Indirect (Displacement) Stages: %u", gen.indStage);
	}

	return true;
}
bool MaterialEditor::drawColorTab(std::vector<libcube::IMaterialDelegate*>& mats, MatDelegate& active_selection)
{
	auto drawColors = [&]() {
		if (!active_selection.support.canRead(p::Feature::MatAmbColor))
			return;

		libcube::gx::ColorF32 mat_a = active_selection.getMatColor(0);
		libcube::gx::ColorF32 mat_b = active_selection.getMatColor(1);
		libcube::gx::ColorF32 amb_a = active_selection.getAmbColor(0);
		libcube::gx::ColorF32 amb_b = active_selection.getAmbColor(1);

		ImGui::PushID(0);
		ImGui::BeginGroup();
			ImGui::Text("Material Colors");
			ImGui::ColorEdit4("A", mat_a);
			ImGui::ColorEdit4("B", mat_b);
		ImGui::EndGroup();
		ImGui::PopID();
		ImGui::PushID(1);
		ImGui::BeginGroup();
			ImGui::Text("Ambient Colors");
			ImGui::ColorEdit4("A", amb_a);
			ImGui::ColorEdit4("B", amb_b);
		ImGui::EndGroup();
		ImGui::PopID();

		if (!active_selection.support.canWrite(p::Feature::MatAmbColor) ||
			(
				mat_a == active_selection.getMatColor(0) &&
				mat_b == active_selection.getMatColor(1) &&
				amb_a == active_selection.getAmbColor(0) &&
				amb_b == active_selection.getAmbColor(1)
			)
		)
			return;
		
		for (auto p : mats)
		{
			p->setMatColor(0, mat_a);
			p->setMatColor(1, mat_b);
			p->setAmbColor(0, amb_a);
			p->setAmbColor(1, amb_b);
		}
	};


	drawColors();


	return true;
}

bool MaterialEditor::drawRight(std::vector<MatDelegate*>& mats, MatDelegate& active)
{
	Predicate<ImGui::EndChild> g;

	if (!ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())))
		return false;

	if (mats.empty())
	{
		ImGui::Text("No selection.");
		return false;
	}

	switch ((MatTab)selected)
	{
	case MatTab::DisplaySurface:
		drawGenInfoTab(mats, active);
		break;
	case MatTab::Color: {
		drawColorTab(mats, active);
		break;
	}
#ifdef DEBUG
	case MatTab::Debug: {

		ImGui::Text("Material Delegate Registration");
		
		const auto toStr = [](p::Coverage p) -> const char* {
			switch (p)
			{
			case p::Coverage::Unsupported:
				return "Unsupported";
			case p::Coverage::Read:
				return "Read";
			case p::Coverage::ReadWrite:
				return "ReadWrite";
			default:
				return "?";
			}
		};

		ImGui::Separator();
		ImGui::Columns(2, "DebugMatFeatures");
			ImGui::Text("Feature"); ImGui::NextColumn();
			ImGui::Text("Support"); ImGui::NextColumn();
		ImGui::Separator();

		static int selected = -1; // FIXME
		for (u32 i = 0; i < (u32)p::Feature::Max; ++i)
		{
			ImGui::Selectable(p::featureStrings[i], selected == i, ImGuiSelectableFlags_SpanAllColumns);
			ImGui::NextColumn();
			ImGui::Text(toStr(active.support[static_cast<p::Feature>(i)]));
			ImGui::NextColumn();
		}
		
		ImGui::Columns(1);
		ImGui::Separator();

		break;
	}
#endif
	default:
		ImGui::Text("?");
		break;
	}
	return true;
}

}
