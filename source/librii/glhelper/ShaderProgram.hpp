#pragma once

#include <core/common.h>
#include <string>

namespace librii::glhelper {

struct ShaderProgram {
  explicit ShaderProgram(const char* vtx, const char* frag);
  explicit ShaderProgram(const std::string& vtx, const std::string& frag);
  ShaderProgram(ShaderProgram&& rhs)
      : mErrorDesc(rhs.mErrorDesc), mShaderProgram(rhs.mShaderProgram),
        bError(rhs.bError) {
    rhs.mShaderProgram = ~0;
  }
  ShaderProgram(const ShaderProgram&) = delete;
  ~ShaderProgram();

  ShaderProgram& operator=(ShaderProgram&& rhs) {
    this->~ShaderProgram(); // Free old shader
    mErrorDesc = rhs.mErrorDesc;
    mShaderProgram = rhs.mShaderProgram;
    bError = rhs.bError;
    rhs.mShaderProgram = ~0;
    return *this;
  }

  u32 getId() const { return mShaderProgram; }
  bool getError() const { return bError; }
  std::string getErrorDesc() const { return mErrorDesc; }

private:
  std::string mErrorDesc;
  u32 mShaderProgram;
  bool bError = false;
};

} // namespace librii::glhelper
