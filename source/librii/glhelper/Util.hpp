#pragma once

namespace librii::glhelper {

bool IsGlWireframeSupported();
void SetGlWireframe(bool wireframe);

void ClearGlScreen();

const char* GetGpuName();
const char* GetGlVersion();

} // namespace librii::glhelper
