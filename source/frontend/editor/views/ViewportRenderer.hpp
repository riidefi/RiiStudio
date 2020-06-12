#pragma once

#include <core/kpi/Node.hpp>                // kpi::IDocumentNode
#include <frontend/editor/StudioWindow.hpp> // StudioWindow

namespace riistudio::frontend {

std::unique_ptr<StudioWindow>
MakeViewportRenderer(const kpi::IDocumentNode& host);

} // namespace riistudio::frontend
