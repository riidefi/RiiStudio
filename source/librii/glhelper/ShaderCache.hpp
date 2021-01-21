#pragma once

#include <librii/glhelper/ShaderProgram.hpp>
#include <map>
#include <memory>
#include <string>

namespace librii::glhelper {

struct ShaderCache {
  static ShaderProgram& compile(const std::string& vert,
                                const std::string& frag);

private:
  static std::map<std::pair<std::string, std::string>,
                  std::unique_ptr<ShaderProgram>>
      mShaders;
};

} // namespace librii::glhelper
