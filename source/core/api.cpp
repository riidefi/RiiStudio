#include "common.h"
#include <core/kpi/Node.hpp>
#include <memory>
#include <oishii/reader/binary_reader.hxx>
#include <vector>

#include <core/kpi/PropertyView.hpp>
#include <core/kpi/RichNameManager.hpp>

kpi::ReflectionMesh* kpi::ReflectionMesh::spInstance;

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
SpawnImporter(const std::string& fileName, oishii::BinaryReader& reader) {
  std::string match = "";
  std::unique_ptr<kpi::IBinaryDeserializer> out = nullptr;

  assert(kpi::ApplicationPlugins::getInstance());
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
std::unique_ptr<kpi::IBinarySerializer>
SpawnExporter(kpi::INode& node) {
  for (const auto& plugin : kpi::ApplicationPlugins::getInstance()->mWriters) {
    if (plugin->canWrite_(node)) {
      return plugin->clone();
    }
  }

  return nullptr;
}
struct DataMesh : public kpi::DataMesh {
  kpi::InternalClassMirror* get(const std::string& id) override {
    const auto found = mClasses.find(id);
    if (found == mClasses.end())
      return nullptr;
    return &found->second;
  }
  void declare(const std::string& id, kpi::RichName name) override {
    mClasses[id].mName = name;
  }
  void enqueueHierarchy(kpi::MirrorEntry entry) override {
    assert(entry.derived != entry.base);
    mToInsert.push(entry);
  }
  void compute() override {
    while (!mToInsert.empty()) {
      const auto& cmd = mToInsert.front();

      // Data mesh and type database are now independent..
      // if (mClasses.find(std::string(cmd.base)) == mClasses.end()) {
      //   printf("Warning: Type %s references undefined parent %s.\n",
      //          cmd.derived.data(), cmd.base.data());
      // }
      //           else
      {
        mClasses[std::string(cmd.base)].derived = cmd.base;
        mClasses[std::string(cmd.base)].mChildren.push_back(
            std::string(cmd.derived));
      }
      // TODO
      mClasses[std::string(cmd.derived)].derived = cmd.derived;

      assert(cmd.derived != cmd.base);

      mClasses[std::string(cmd.derived)].mParents.push_back(
          {std::string(cmd.base), cmd.translation});
      mToInsert.pop();
    }
  }

private:
  // File states and interfaces -- hierarchy.
  std::map<std::string, kpi::InternalClassMirror> mClasses;
  std::queue<kpi::MirrorEntry> mToInsert;
};

struct ReflectionMesh : public kpi::ReflectionMesh {
  using kpi::ReflectionMesh::ReflectionMesh;

  ReflectionInfoHandle lookupInfo(std::string info) override {
    return ReflectionInfoHandle(&getDataMesh(), info);
  }
  void findParentOfType(std::vector<void*>& out, void* in,
                        const std::string& info,
                        const std::string& key) override {
    auto hnd = ReflectionInfoHandle(&getDataMesh(), info);

    for (int i = 0; i < hnd.getNumParents(); ++i) {
      auto parent = hnd.getParent(i);
      assert(hnd.getName() != parent.getName());
      if (parent.getName() == key) {
        char* new_ =
            reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i);
        if (std::find(out.begin(), out.end(), (void*)new_) == out.end())
          out.push_back(new_);
      } else
        findParentOfType(
            out, reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i),
            parent.getName(), key);
    }
  }
};

void InitAPI() {
  kpi::ReflectionMesh::setInstance(
      new ReflectionMesh(std::make_unique<DataMesh>()));

  // Register plugins
  for (auto* it = kpi::RegistrationLink::getHead(); it != nullptr;
       it = it->getLast()) {
    it->exec(*kpi::ApplicationPlugins::getInstance());
  }

  kpi::ReflectionMesh::getInstance()->getDataMesh().compute();
}
void DeinitAPI() {
  delete kpi::ReflectionMesh::getInstance();
}

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
