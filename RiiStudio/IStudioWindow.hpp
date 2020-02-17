#pragma once

#include <LibCore/windows/Window.hpp>
#include <LibCore/windows/WindowManager.hpp>
#include <LibCore/windows/SelectionManager.hpp>

#include <ThirdParty/imgui/imgui.h>
#include <string>
#include <imgui/imgui_internal.h>

class IStudioWindow : public WindowManager
{
protected:
    // Parents can't change in studio windows.
	virtual void draw() noexcept {}



public:
    IStudioWindow* getParent() { return dynamic_cast<IStudioWindow*>(mParent); }
	void setParent(Window* parent) { mParent = parent; }
	const ImGuiWindowClass* getWindowClass() { return &mWindowClass; }

	IStudioWindow(const std::string& name, bool dockspace=false)
		: mName(name), mbDrawDockspace(dockspace)
	{
		mId = ImGui::GetID(name.c_str()); // TODO
		mWindowClass.DockingAllowUnclassed = false;
		mWindowClass.ClassId = mId;
	}
    void setWindowFlag(u32 flag)
	{
		mFlags |= flag;
	}

	std::string getName() const { return mName; }

private:
	std::string idIfy(std::string in)
	{
		return in + "##";
	}
    void draw(Window* pWin) noexcept override
    {
        mParent = pWin;

		if (getParent())
			ImGui::SetNextWindowClass(getParent()->getWindowClass());
		if (ImGui::Begin(idIfy(mName).c_str(), &bOpen, mFlags))
		{
			ImGui::PushID(mId);

			if (mbDrawDockspace)
			{
				const ImGuiID dockspaceId = ImGui::GetID("###DockSpace");
				if (!ImGui::DockBuilderGetNode(dockspaceId)) {
					ImGui::DockBuilderRemoveNode(dockspaceId);
					ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);
				
					ImGuiID next = dockspaceId;
					ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 0.2f, nullptr, &next);
					ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.2f, nullptr, &dock_right_id);

					ImGui::DockBuilderDockWindow(idIfy("Outliner").c_str(), dock_right_id);
					ImGui::DockBuilderDockWindow(idIfy("Texture Preview").c_str(), dock_right_down_id);
					ImGui::DockBuilderDockWindow(idIfy("Viewport").c_str(), next);
				
				
					ImGui::DockBuilderFinish(dockspaceId);
				}
				ImGui::DockSpace(dockspaceId, {}, ImGuiDockNodeFlags_CentralNode, getWindowClass());
			}
			draw();
			drawChildren(this);
			processWindowQueue();

			if (ImGui::IsItemActive())
				pWin->setActiveWindow(this);

			ImGui::PopID();
		}
		ImGui::End();
    }
	ImGuiWindowClass mWindowClass;
	std::string mName;
	ImGuiID mId;
	bool mbDrawDockspace = true;
	u32 mFlags = 0;
    // TODO: Selection
};
