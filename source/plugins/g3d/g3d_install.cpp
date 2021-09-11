#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <core/kpi/Plugins.hpp>
#include <core/kpi/RichNameManager.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include "G3dIo.hpp"
#include "collection.hpp"
#include "model.hpp"

namespace riistudio::g3d {

class BRRES_IO {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {
    return file.ends_with("brres") ? typeid(Collection).name() : "";
  }
  void read(kpi::IOTransaction& transaction) const {
    assert(dynamic_cast<Collection*>(&transaction.node) != nullptr);

    Collection& collection = *dynamic_cast<Collection*>(&transaction.node);
    oishii::BinaryReader reader(std::move(transaction.data));

    ReadBRRES(collection, reader, transaction);
  }
  bool canWrite(kpi::INode& node) const {
    return dynamic_cast<Collection*>(&node) != nullptr;
  }
  void write(kpi::INode& node, oishii::Writer& writer) const {
    assert(dynamic_cast<Collection*>(&node) != nullptr);
    Collection& collection = *dynamic_cast<Collection*>(&node);

    WriteBRRES(collection, writer);
  }
};

void InstallG3d(kpi::ApplicationPlugins& installer) {
  installer.addType<g3d::Bone, libcube::IBoneDelegate>()
      .addType<g3d::Texture, libcube::Texture>()
      .addType<g3d::Material, libcube::IGCMaterial>()
      .addType<g3d::Model, lib3d::Model>()
      .addType<g3d::Collection, lib3d::Scene>()
      .addType<g3d::PositionBuffer>()
      .addType<g3d::NormalBuffer>()
      .addType<g3d::ColorBuffer>()
      .addType<g3d::TextureCoordinateBuffer>()
      .addType<g3d::Polygon, libcube::IndexedPolygon>()
      .addType<g3d::SRT0>();

  kpi::RichNameManager& rich = kpi::RichNameManager::getInstance();
  rich.addRichName<g3d::ColorBuffer>((const char*)ICON_FA_BRUSH,
                                     "Vertex Color");
  rich.addRichName<g3d::SRT0>((const char*)ICON_FA_WAVE_SQUARE,
                              "Texture Matrix Animation");

  installer.addDeserializer<BRRES_IO>();
  installer.addSerializer<BRRES_IO>();
}

} // namespace riistudio::g3d
