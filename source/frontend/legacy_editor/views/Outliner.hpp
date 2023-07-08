#pragma once

#include <LibBadUIFramework/Node2.hpp> // kpi::INode
#include <memory>                      // std::unique_ptr

namespace kpi {
class IDocumentNode;
struct SelectionManager;
} // namespace kpi

namespace riistudio::lib3d {
struct Texture;
}

namespace riistudio::frontend {

class StudioWindow;

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, kpi::SelectionManager& active,
             std::function<void(const lib3d::Texture*, u32)> icon,
             std::function<void()> post, std::function<void(bool)> commit);

} // namespace riistudio::frontend
