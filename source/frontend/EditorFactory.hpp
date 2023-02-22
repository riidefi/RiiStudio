#pragma once

#include <frontend/file_host.hpp>
#include <frontend/window/window.hpp>

namespace riistudio::frontend {

std::unique_ptr<IWindow> MakeEditor(FileData& data);

std::optional<std::vector<uint8_t>> LoadLuigiCircuitSample();

} // namespace riistudio::frontend
