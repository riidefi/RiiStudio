#pragma once

#include <RiiStudio/editor/BoneEditor.hpp>
#include <ThirdParty/imgui/imgui.h>


namespace ed {

template<void(*PredFunc)(void)>
struct Predicate
{
	~Predicate() { PredFunc(); }
};
void BoneEditor::drawBone(libcube::GCCollection::IBoneDelegate& d, bool leaf)
{
	using features = libcube::GCCollection::BoneFeatures;
	bool o = ImGui::TreeNodeEx((void*)&d, ImGuiTreeNodeFlags_DefaultOpen | (d.getChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0), d.getNameCStr());
	ImGui::NextColumn();
	{
#define BONE_PROP(ID, D) \
	ImGui::PushID(ID); ImGui::PushItemWidth(-1); \
	D; \
	ImGui::PopItemWidth(); ImGui::PopID(); ImGui::NextColumn();

		// Draw bone contents
		auto srt = d.getSRT();
		BONE_PROP("Scale", ImGui::InputFloat3("", &srt.scale.x));
		BONE_PROP("Rotation", ImGui::InputFloat3("", &srt.rotation.x));
		BONE_PROP("Translation", ImGui::InputFloat3("", &srt.translation.x));

		if (d.canWrite(features::SRT) && srt != d.getSRT())
			d.setSRT(srt);

		int sel = static_cast<int>(d.getBillboard());
		BONE_PROP("Billboard", ImGui::Combo("", &sel, "None\0Z_Parallel\0Z_Pesrp\0Z_ParallelRotate\0Z_PerspRotate\0Y_Parallel\0Y_Persp\0J3D_XY\0J3D_Y"));

		// TODO -- Doesn't hide features that shouldn't be editable!
		if ((d.canWrite(features::StandardBillboards) || d.canWrite(features::ExtendedBillboards)) && sel != static_cast<int>(d.getBillboard()))
			d.setBillboard(static_cast<libcube::GCCollection::IBoneDelegate::Billboard>(sel));

		bool b = d.getSSC();
		BONE_PROP("Segment Scale Compensation", ImGui::Checkbox("", &b));
		if (d.canWrite(features::SegmentScaleCompensation) && d.getSSC() != b)
			d.setSSC(b);
#undef BONE_PROP
	}
	if (o)
	{
		for (const auto& s : d.getChildren())
		{
			int i = samp.boneNameToIdx(s);
			assert(i >= 0 && i < samp.getNumBones());
			if (i >= 0 && i < samp.getNumBones())
			{
				assert(samp.getBoneDelegate(i).getParent() == d.getNameCStr());
				drawBone(samp.getBoneDelegate(i));
			}

		}
		ImGui::TreePop();
	}
};
void BoneEditor::draw(WindowContext* ctx) noexcept
{
	Predicate<ImGui::End> g;

	if (!ImGui::Begin("BoneEditor", &bOpen) || !ctx)
		return;

	if (ImGui::TreeNodeEx("(root)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::NextColumn();
		ImGui::Columns(6, "tree", true);
		ImGui::Text("Name");
		ImGui::NextColumn();
		ImGui::Text("Scale");
		ImGui::NextColumn();
		ImGui::Text("Rotation");
		ImGui::NextColumn();
		ImGui::Text("Translation");
		ImGui::NextColumn();
		ImGui::Text("Billboard");
		ImGui::NextColumn();
		ImGui::Text("Segment Scale Compensation");
		ImGui::NextColumn();

		drawBone(samp.getBoneDelegate(0));
		
		ImGui::Columns(1);
		ImGui::TreePop();
	}

}

} // namespace ed
