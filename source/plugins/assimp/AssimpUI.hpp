#pragma once

#include <LibBadUIFramework/Plugins.hpp>

namespace riistudio::assimp {

std::unique_ptr<kpi::IBinaryDeserializer> CreatePlugin();

} // namespace riistudio::assimp
