#pragma once

#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/data/TextureData.hpp>
#include <librii/g3d/io/AnimChrIO.hpp>
#include <librii/g3d/io/AnimClrIO.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/AnimTexPatIO.hpp>
#include <librii/g3d/io/AnimVisIO.hpp>

// Regrettably, for kpi::LightIOTransaction
#include <LibBadUIFramework/Plugins.hpp>

namespace librii::g3d {

struct DumpResult {
  std::string jsonData;
  std::vector<u8> collatedBuffer;
};

struct Archive;

DumpResult DumpJson(const librii::g3d::Archive& archive);

} // namespace librii::g3d
