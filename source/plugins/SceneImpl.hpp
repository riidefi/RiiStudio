#include <core/3d/i3dmodel.hpp>
#include <LibBadUIFramework/Node2.hpp>
#include <glm/mat4x4.hpp>
#include <librii/g3d/gfx/G3dGfx.hpp>
#include <librii/gfx/SceneState.hpp>

namespace riistudio::lib3d {

class SceneImpl : public IDrawable {
public:
  SceneImpl() = default;
  virtual ~SceneImpl() = default;

  Result<void> prepare(SceneState& state, const Scene& host, glm::mat4 v_mtx,
                       glm::mat4 p_mtx, RenderType type) override;

  void gatherBoneRecursive(SceneBuffers& output, u64 boneId,
                           const lib3d::Model& root, const lib3d::Scene& scn,
                           glm::mat4 v_mtx, glm::mat4 p_mtx);

  void gather(SceneBuffers& output, const lib3d::Model& root,
              const lib3d::Scene& scene, glm::mat4 v_mtx, glm::mat4 p_mtx);

private:
  librii::g3d::gfx::G3dSceneRenderData render_data;
  bool uploaded = false;
};

} // namespace riistudio::lib3d
