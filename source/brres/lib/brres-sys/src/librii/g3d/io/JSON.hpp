#include <rsl/Result.hpp>
#include <string>
#include <string_view>

namespace librii::g3d {

struct Model;

std::string ModelToJSON(const Model& model);
Result<Model> JSONToModel(std::string_view json);

} // namespace librii::g3d
