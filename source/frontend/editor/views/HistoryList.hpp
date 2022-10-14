#pragma once

#include <core/kpi/History.hpp>             // kpi::History
#include <core/kpi/Node2.hpp>               // kpi::IDocumentNode
#include <frontend/editor/StudioWindow.hpp> // StudioWindow

namespace riistudio::frontend {

std::unique_ptr<StudioWindow> MakeHistoryList(kpi::History& host,
                                              kpi::INode& root,
                                              kpi::SelectionManager& sel);

} // namespace riistudio::frontend
