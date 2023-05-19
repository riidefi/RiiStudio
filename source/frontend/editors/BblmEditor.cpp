#include <frontend/widgets/ReflectedPropertyGrid.hpp>

#include "BblmEditor.hpp"

namespace riistudio::frontend {

void BblmEditorPropertyGrid::Draw() {
  ReflectedPropertyGrid grid;
  grid.Draw(m_blm);
}

} // namespace riistudio::frontend
