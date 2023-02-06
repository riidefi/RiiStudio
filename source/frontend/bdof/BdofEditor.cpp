#include <frontend/widgets/ReflectedPropertyGrid.hpp>

#include "BdofEditor.hpp"

namespace riistudio::frontend {

void BdofEditorPropertyGrid::Draw() {
  ReflectedPropertyGrid grid;
  grid.Draw(m_dof);
}

} // namespace riistudio::frontend
