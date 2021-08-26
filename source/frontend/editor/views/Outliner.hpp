#pragma once

#include <core/kpi/Node2.hpp> // kpi::INode
#include <memory>             // std::unique_ptr

namespace kpi {
class IDocumentNode;
} // namespace kpi

namespace riistudio::frontend {

class EditorWindow;
class StudioWindow;

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, kpi::IObject*& active, EditorWindow& ed);


} // namespace riistudio::frontend
