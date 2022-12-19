#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <core/kpi/Plugins.hpp>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

#include <librii/g3d/io/DictWriteIO.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

#include <set>
#include <string>

#include "Common.hpp"

#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/ArchiveIO.hpp>

#include <ranges>

namespace librii::g3d {
using namespace bad;
}

namespace riistudio::g3d {

// MDL0.cpp
librii::g3d::BinaryModel toBinaryModel(const Model& mdl);
void processModel(librii::g3d::BinaryModel& binary_model,
                  kpi::LightIOTransaction& transaction,
                  const std::string& transaction_path,
                  riistudio::g3d::Model& mdl);

void ReadBRRES(Collection& collection, oishii::BinaryReader& reader,
               kpi::LightIOTransaction& transaction) {
  librii::g3d::BinaryArchive archive;
  archive.read(reader, transaction);
  collection.path = reader.getFile();
  for (auto& mdl : archive.models) {
    auto& editor_mdl = collection.getModels().add();
    processModel(mdl, transaction, "MDL0 " + mdl.name, editor_mdl);
  }
  for (auto& tex : archive.textures) {
    static_cast<librii::g3d::TextureData&>(collection.getTextures().add()) =
        tex;
  }
  for (auto& srt : archive.srts) {
    static_cast<librii::g3d::SrtAnimationArchive&>(
        collection.getAnim_Srts().add()) = srt;
  }
}


void WriteBRRES(Collection& collection, oishii::Writer& writer) {
  librii::g3d::BinaryArchive arc;
  auto range = collection.getModels() | std::views::transform(toBinaryModel);
  arc.models = {range.begin(), range.end()};
  arc.textures = {collection.getTextures().begin(),
                  collection.getTextures().end()};
  arc.srts = {collection.getAnim_Srts().begin(),
              collection.getAnim_Srts().end()};
  arc.write(writer);
}

} // namespace riistudio::g3d
