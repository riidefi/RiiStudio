#include "LRAssimp.hpp"
#include <json_struct/json_struct.h>

using namespace librii::lra;

JS_OBJ_EXT(glm::vec2, x, y);
JS_OBJ_EXT(glm::vec3, x, y, z);
JS_OBJ_EXT(glm::vec4, x, y, z, w);
JS_OBJ_EXT(SceneAttrs, Incomplete, Validated, HasValidationIssues,
           HasSharedVertices);
JS_OBJ_EXT(Face, indices);
JS_OBJ_EXT(Mesh, positions, normals, colors, uvs, faces, materialIndex, min,
           max);
JS_OBJ_EXT(Material, name, texture);
JS_OBJ_EXT(Node, name, xform, children, meshes);
JS_OBJ_EXT(Animation, name, frameCount, fps);
JS_OBJ_EXT(Scene, flags, nodes, meshes, materials, animations);
namespace JS {
template <> struct TypeHandler<glm::mat4> {
public:
  static inline Error to(glm::mat4& to_type, ParseContext& context) {
    return Error::NoError;
  }
  static void from(const glm::mat4& from_type, Token& token,
                   Serializer& serializer) {}
};
template <typename T> struct TypeHandler<std::array<T, 8>> {
  static inline Error to(auto& to_type, ParseContext& context) {
    return Error::NoError;
  }

  static inline void from(auto& vec, Token& token, Serializer& serializer) {
    token.value_type = Type::ArrayStart;
    token.value = DataRef("[");
    serializer.write(token);

    token.name = DataRef("");

    for (auto& index : vec) {
      TypeHandler<T>::from(index, token, serializer);
    }

    token.name = DataRef("");

    token.value_type = Type::ArrayEnd;
    token.value = DataRef("]");
    serializer.write(token);
  }
};
} // namespace JS

namespace librii::lra {

std::string PrintJSON(const Scene& scn) { return JS::serializeStruct(scn); }

} // namespace librii::lra
