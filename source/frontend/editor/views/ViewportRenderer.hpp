#pragma once

#include <core/kpi/Node2.hpp>               // kpi::IDocumentNode
#include <frontend/editor/StudioWindow.hpp> // StudioWindow

namespace riistudio::frontend {

std::unique_ptr<StudioWindow> MakeViewportRenderer(const kpi::INode& host);

} // namespace riistudio::frontend
