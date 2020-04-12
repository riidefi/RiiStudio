#pragma once

#include <core/common.h>
#include <string>

struct ShaderProgram {
  explicit ShaderProgram(const char* vtx, const char* frag);
  explicit ShaderProgram(const std::string& vtx, const std::string& frag);
  ~ShaderProgram();

  u32 getId() const { return mShaderProgram; }

private:
  u32 mShaderProgram;
};
