#ifdef _WIN32
#include <Windows.h>
#include <string>
#endif

#include "Node.hpp"
#include "Node2.hpp"
#include "Plugins.hpp"
#include <core/common.h>

namespace kpi {

ApplicationPlugins ApplicationPlugins::sInstance;
RegistrationLink* RegistrationLink::gRegistrationChain = nullptr;

#ifdef _WIN32

using riimain_fn_t = void(CALLBACK*)(ApplicationPlugins*);

bool installModuleNative(const std::string& path,
                         ApplicationPlugins* pInstaller) {
  auto hnd = LoadLibraryA(path.c_str());

  if (!hnd)
    return false;

  auto fn = reinterpret_cast<riimain_fn_t>(GetProcAddress(hnd, "__riimain"));
  if (!fn)
    return false;

  fn(pInstaller);
  return true;
}
#else
#warning "No module installation support.."
template <typename... args> bool installModuleNative(args...) { return false; }
#endif

void ApplicationPlugins::registerMirror(const MirrorEntry& entry) {
  ReflectionMesh::getInstance()->getDataMesh().enqueueHierarchy(entry);
}
void ApplicationPlugins::installModule(const std::string& path) {
  if (path.ends_with(".dll"))
    installModuleNative(path, this);
}

// TODO: Cleanup
std::unique_ptr<IObject>
ApplicationPlugins::spawnState(const std::string& type) const {
  for (const auto& it : mFactories) {
    if (it.first == type) {
      auto doc = it.second->spawn();
      // doc->mType = type;
      return std::move(doc);
    }
  }

  assert(!"Failed to spawn state..");
  throw "Cannot spawn";
  return nullptr;
}
std::unique_ptr<IObject>
ApplicationPlugins::constructObject(const std::string& type,
                                    INode* parent) const {
  auto spawned = spawnState(type);
  // spawned->parent = parent;
  return spawned;
}

} // namespace kpi
