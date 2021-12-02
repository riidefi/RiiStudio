#pragma once

#include <core/kpi/Plugins.hpp>
#include <memory>

namespace riistudio::ass {

std::unique_ptr<kpi::IBinaryDeserializer> CreatePlugin();

} // namespace riistudio::ass
