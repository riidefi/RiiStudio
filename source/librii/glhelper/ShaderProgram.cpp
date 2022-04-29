#include "ShaderProgram.hpp"
#include <core/3d/gl.hpp>
#include <iostream>

namespace librii::glhelper {

#ifdef RII_GL
bool checkShaderErrors(u32 id, std::string& error) {
  s32 success;
  char infoLog[512];
  glGetShaderiv(id, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(id, 512, NULL, infoLog);
    printf("Shader compilation failed: %s\n", infoLog);
    error += infoLog;
  }

  return success;
}

ShaderProgram::ShaderProgram(const char* vtx, const char* frag) {
  assert(vtx != nullptr && *vtx != '\0');
  assert(frag != nullptr && *frag != '\0');

  const u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vtx, NULL);
  glCompileShader(vertexShader);

  const u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &frag, NULL);
  glCompileShader(fragmentShader);

  if (!checkShaderErrors(vertexShader, mErrorDesc)) {
    bError = true;
    printf("%s\n", vtx);
  }
  if (!checkShaderErrors(fragmentShader, mErrorDesc)) {
    bError = true;
    printf("%s\n", frag);
    // mErrorDesc = frag;
  }
  mShaderProgram = glCreateProgram();
  glAttachShader(mShaderProgram, vertexShader);
  glAttachShader(mShaderProgram, fragmentShader);
  glLinkProgram(mShaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}
ShaderProgram::ShaderProgram(const std::string& vtx, const std::string& frag)
    : ShaderProgram(vtx.c_str(), frag.c_str()) {}
ShaderProgram::~ShaderProgram() {
#ifndef RII_PLATFORM_EMSCRIPTEN
  if (mShaderProgram != ~0)
    glDeleteProgram(mShaderProgram);
#endif
}
#endif

} // namespace librii::glhelper
