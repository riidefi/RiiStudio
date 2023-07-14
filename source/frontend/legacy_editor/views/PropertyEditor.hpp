#pragma once

#include <LibBadUIFramework/History.hpp>           // kpi::History
#include <LibBadUIFramework/Node2.hpp>             // kpi::IDocumentNode
#include <LibBadUIFramework/PropertyView.hpp>      // PropertyViewStateHolder
#include <frontend/legacy_editor/EditorWindow.hpp> // EditorWindow
#include <frontend/legacy_editor/StudioWindow.hpp> // StudioWindow

namespace riistudio::frontend {

std::unique_ptr<StudioWindow>
MakePropertyEditor(kpi::History& host, kpi::INode& root,
                   std::function<std::set<kpi::IObject*>()> selection,
                   std::function<kpi::IObject*()> selectionActive,
                   std::function<void(const lib3d::Texture*, u32)> ed);

} // namespace riistudio::frontend
