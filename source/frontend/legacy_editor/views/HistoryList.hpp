#pragma once

#include <LibBadUIFramework/History.hpp>           // kpi::History
#include <LibBadUIFramework/Node2.hpp>             // kpi::IDocumentNode
#include <frontend/legacy_editor/StudioWindow.hpp> // StudioWindow

namespace riistudio::frontend {

std::unique_ptr<StudioWindow> MakeHistoryList(kpi::History& host,
                                              kpi::INode& root,
                                              kpi::SelectionManager& sel);

} // namespace riistudio::frontend
