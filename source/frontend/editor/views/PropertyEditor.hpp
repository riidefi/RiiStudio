#pragma once

#include <core/kpi/History.hpp>             // kpi::History
#include <core/kpi/Node.hpp>                // kpi::IDocumentNode
#include <core/kpi/PropertyView.hpp>        // PropertyViewStateHolder
#include <frontend/editor/EditorWindow.hpp> // EditorWindow
#include <frontend/editor/StudioWindow.hpp> // StudioWindow

namespace riistudio::frontend {

std::unique_ptr<StudioWindow> MakePropertyEditor(kpi::History& host,
                                                 kpi::INode& root,
                                                 kpi::IObject*& active,
                                                 EditorWindow& ed);

} // namespace riistudio::frontend
