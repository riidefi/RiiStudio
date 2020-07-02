#include "Reflection.hpp"
#include <assert.h>

namespace kpi {

ReflectionMesh ReflectionMesh::sInstance;

struct DataMeshImpl : public kpi::DataMesh {
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

ReflectionMesh::ReflectionMesh()
    : mDataMesh(std::make_unique<DataMeshImpl>()) {}

ReflectionMesh::ReflectionInfoHandle
ReflectionMesh::lookupInfo(std::string info) {
  return ReflectionInfoHandle(&getDataMesh(), info);
}
void ReflectionMesh::findParentOfType(std::vector<void*>& out, void* in,
                                      const std::string& info,
                                      const std::string& key) {
  auto hnd = ReflectionInfoHandle(&getDataMesh(), info);

  for (int i = 0; i < hnd.getNumParents(); ++i) {
    auto parent = hnd.getParent(i);
    assert(hnd.getName() != parent.getName());
    if (parent.getName() == key) {
      char* new_ = reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i);
      if (std::find(out.begin(), out.end(), (void*)new_) == out.end())
        out.push_back(new_);
    } else
      findParentOfType(
          out, reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i),
          parent.getName(), key);
  }
}

} // namespace kpi
