#include "polygon.hpp"

#include "model.hpp"

namespace riistudio::g3d {

glm::vec2 Polygon::getUv(u64 chan, u64 id) const {
  // Assumption: Parent of parent model is a collection with children.
  const kpi::IDocumentNode *parent = getParent();
  assert(parent);
  const auto *posBufs = parent->getFolder<TextureCoordinateBuffer>();
  assert(posBufs);
  for (const auto &p : *posBufs) {
    if (p->getName() == mTexCoordBuffer[chan]) {
      auto *x = dynamic_cast<TextureCoordinateBuffer *>(p.get());
      return x->mEntries[id];
    }
  }
  return {};
}
glm::vec4 Polygon::getClr(u64 id) const {
  const kpi::IDocumentNode *parent = getParent();
  assert(parent);
  const auto *posBufs = parent->getFolder<ColorBuffer>();
  assert(posBufs);
  for (const auto &p : *posBufs) {
    if (p->getName() == mColorBuffer[0]) {
      const auto *buf = dynamic_cast<ColorBuffer *>(p.get());
      const auto entry = buf->mEntries[id];
      const libcube::gx::ColorF32 ef32 = entry;
      return ef32;
    }
  }
  return {};
}
glm::vec3 Polygon::getPos(u64 id) const {
  const kpi::IDocumentNode *parent = getParent();
  assert(parent);
  const auto *posBufs = parent->getFolder<PositionBuffer>();
  assert(posBufs);
  for (const auto &p : *posBufs) {
    if (p->getName() == mPositionBuffer)
      return dynamic_cast<PositionBuffer *>(p.get())->mEntries[id];
  }
  return {};
}
glm::vec3 Polygon::getNrm(u64 id) const {
  const kpi::IDocumentNode *parent = getParent();
  assert(parent);
  const auto *posBufs = parent->getFolder<NormalBuffer>();
  assert(posBufs);
  for (const auto &p : *posBufs) {
    if (p->getName() == mNormalBuffer)
      return dynamic_cast<NormalBuffer *>(p.get())->mEntries[id];
  }
  return {};
}
u64 Polygon::addPos(const glm::vec3 &v) { return 0; }
u64 Polygon::addNrm(const glm::vec3 &v) { return 0; }
u64 Polygon::addClr(u64 chan, const glm::vec4 &v) { return 0; }
u64 Polygon::addUv(u64 chan, const glm::vec2 &v) { return 0; }

void Polygon::addTriangle(std::array<SimpleVertex, 3> tri) {}

} // namespace riistudio::g3d
