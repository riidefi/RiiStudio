#pragma once

#include <core/kpi/Node2.hpp> // kpi::INode
#include <memory>             // std::unique_ptr

namespace kpi {
class IDocumentNode;
} // namespace kpi

namespace riistudio::frontend {

class EditorWindow;
struct SelectionManager;
class StudioWindow;

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, SelectionManager& active, EditorWindow& ed);

} // namespace riistudio::frontend
