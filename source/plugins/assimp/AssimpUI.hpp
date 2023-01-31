#pragma once

#include <core/kpi/Plugins.hpp>

namespace riistudio::assimp {

std::unique_ptr<kpi::IBinaryDeserializer> CreatePlugin();

} // namespace riistudio::assimp
