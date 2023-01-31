#include <core/common.h>
#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <LibBadUIFramework/RichNameManager.hpp>
#include <oishii/reader/binary_reader.hxx>

#include <memory>
#include <vector>

#include <plugins/assimp/AssimpUI.hpp>

extern void InstallGC();

namespace riistudio::g3d {
void InstallG3d(kpi::ApplicationPlugins& installer);
}

namespace riistudio::j3d {
void InstallJ3d(kpi::ApplicationPlugins& installer);
}

std::unique_ptr<kpi::IObject> SpawnState(const std::string& type) {
  return kpi::ApplicationPlugins::getInstance()->constructObject(type, nullptr);
}
bool IsConstructible(const std::string& type) {
  const auto& factories = kpi::ApplicationPlugins::getInstance()->mFactories;
  return std::find_if(factories.begin(), factories.end(), [&](const auto& it) {
           return it.second->getId() == type;
         }) != factories.end();
}
kpi::RichName GetRich(const std::string& type) {
  const auto* got = kpi::ReflectionMesh::getInstance()->getDataMesh().get(type);
  assert(got);
  return got->mName;
}

std::pair<std::string, std::unique_ptr<kpi::IBinaryDeserializer>>
SpawnImporter(const std::string& fileName, oishii::ByteView data) {
  std::string match = "";
  std::unique_ptr<kpi::IBinaryDeserializer> out = nullptr;

  assert(kpi::ApplicationPlugins::getInstance());
  // Create a child view for intiial check
  oishii::ByteView datacopy(data, data);
  oishii::BinaryReader reader(std::move(datacopy));
  for (const auto& plugin : kpi::ApplicationPlugins::getInstance()->mReaders) {
    oishii::JumpOut reader_guard(reader, reader.tell());
    match = plugin->canRead_(fileName, reader);
    if (!match.empty()) {
      out = plugin->clone();
      break;
    }
  }

  if (match.empty()) {
    DebugReport("No matches.\n");
    return {};
  } else {
    DebugReport("Success spawning importer\n");
    return {match, std::move(out)};
  }
}
std::unique_ptr<kpi::IBinarySerializer> SpawnExporter(kpi::INode& node) {
  for (const auto& plugin : kpi::ApplicationPlugins::getInstance()->mWriters) {
    if (plugin->canWrite_(node)) {
      return plugin->clone();
    }
  }

  return nullptr;
}

void InitAPI() {
  InstallGC();

  auto& installer = *kpi::ApplicationPlugins::getInstance();

  riistudio::g3d::InstallG3d(installer);
  riistudio::j3d::InstallJ3d(installer);

  installer.mReaders.push_back(riistudio::assimp::CreatePlugin());

  // TODO: Phase out this system

  // Register plugins
  for (auto* it = kpi::RegistrationLink::getHead(); it != nullptr;
       it = it->getLast()) {
    it->exec(installer);
  }

  kpi::ReflectionMesh::getInstance()->getDataMesh().compute();
}
void DeinitAPI() {}

std::vector<std::string> GetChildrenOfType(const std::string& type) {
  std::vector<std::string> out;
  const auto hnd = kpi::ReflectionMesh::getInstance()->lookupInfo(type);

  for (int i = 0; i < hnd.getNumChildren(); ++i) {
    out.push_back(hnd.getChild(i).getName());
    for (const auto& str : GetChildrenOfType(hnd.getChild(i).getName()))
      out.push_back(str);
  }
  return out;
}
