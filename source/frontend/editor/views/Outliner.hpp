#pragma once

#include <memory> // std::unique_ptr

namespace kpi {
class IDocumentNode;
} // namespace kpi

namespace riistudio::frontend {

class EditorWindow;
class StudioWindow;

std::unique_ptr<StudioWindow> MakeOutliner(kpi::IDocumentNode& host,
                                           kpi::IDocumentNode*& active,
                                           EditorWindow& ed);

} // namespace riistudio::frontend
