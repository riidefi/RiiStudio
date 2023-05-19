#pragma once

#include <LibBadUIFramework/Node2.hpp> // kpi::IDocumentNode
#include <core/3d/i3dmodel.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp> // StudioWindow

namespace riistudio::frontend {

std::unique_ptr<StudioWindow> MakeViewportRenderer(const lib3d::Scene& host);

} // namespace riistudio::frontend
