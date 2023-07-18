#pragma once

#include <frontend/file_host.hpp>
#include <frontend/window/window.hpp>

namespace riistudio::frontend {

std::unique_ptr<IWindow> MakeEditor(FileData& data);
std::unique_ptr<IWindow> MakeEditor(std::span<const u8> data,
                                    std::string_view path);

std::optional<std::vector<uint8_t>> LoadLuigiCircuitSample();

} // namespace riistudio::frontend
