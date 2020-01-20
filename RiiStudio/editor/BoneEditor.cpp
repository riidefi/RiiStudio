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
	using features = lib3d::BoneFeatures;
	bool o = ImGui::TreeNodeEx((void*)&d, ImGuiTreeNodeFlags_DefaultOpen | (!d.getNumChildren() ? ImGuiTreeNodeFlags_Leaf : 0), d.getName().c_str());
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

		if (samp.getNumBones() > 0 && samp.getBone(0).supportsBoneFeature(lib3d::BoneFeatures::SegmentScaleCompensation) != lib3d::Coverage::Unsupported)
		{
			bool b = d.getSSC();
			BONE_PROP("Segment Scale Compensation", ImGui::Checkbox("", &b));
			if (d.canWrite(features::SegmentScaleCompensation) && d.getSSC() != b)
				d.setSSC(b);
		}
#undef BONE_PROP
	}
	if (o)
	{	
		for (u32 i = 0; i < d.getNumChildren(); ++i)
		{
			assert(samp.getBone(d.getChild(i)).getParent() == d.getId());
			drawBone(samp.getBone(d.getChild(i)));
		}
		ImGui::TreePop();
	}
};
void BoneEditor::draw(WindowContext* ctx) noexcept
{
	Predicate<ImGui::End> g;

	if (!ImGui::Begin("BoneEditor", &bOpen) || !ctx)
		return;

	bool use_ssc = samp.getNumBones() > 0 && samp.getBone(0).supportsBoneFeature(lib3d::BoneFeatures::SegmentScaleCompensation) != lib3d::Coverage::Unsupported;

	if (ImGui::TreeNodeEx("(root)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::NextColumn();
		ImGui::Columns(5 + (use_ssc ? 1 : 0), "tree", true);
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
		if (use_ssc)
		{
			ImGui::Text("Segment Scale Compensation");
			ImGui::NextColumn();
		}
		if (samp.getNumBones() > 0)
			drawBone(samp.getBone(0));
		
		ImGui::Columns(1);
		ImGui::TreePop();
	}

}

} // namespace ed
