#pragma once

#include <core/common.h>
#include <string>

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
    mErrorDesc = rhs.mErrorDesc;
    mShaderProgram = rhs.mShaderProgram;
    bError = rhs.bError;
    rhs.mShaderProgram = ~0;
    return *this;
  }

  u32 getId() const { return mShaderProgram; }
  bool getError() const { return bError; }
  const std::string& getErrorDesc() { return mErrorDesc; }

private:
  std::string mErrorDesc;
  u32 mShaderProgram;
  bool bError = false;
};
