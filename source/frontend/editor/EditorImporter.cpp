#include "EditorImporter.hpp"
#include <core/api.hpp>
#include <oishii/data_provider.hxx>

// TODO
#include <plugins/j3d/Scene.hpp>

namespace riistudio::frontend {

EditorImporter::EditorImporter(FileData&& data) { // TODO: Not ideal..
  std::vector<u8> vec(data.mLen);
  memcpy(vec.data(), data.mData.get(), data.mLen);

  provider = std::make_unique<oishii::DataProvider>(std::move(vec), data.mPath);
  auto [_data_id, _importer] = SpawnImporter(data.mPath, provider->slice());
  data_id = std::move(_data_id);
  mDeserializer = std::move(_importer);

  if (!mDeserializer) {
    result = State::NotImportable;
    return;
  }

  // TODO: Perhaps we might want to support more specialized children of a
  // concrete class?
  if (!IsConstructible(data_id)) {
    const auto children = GetChildrenOfType(data_id);
    if (children.empty()) {
      result = State::NotConstructible;
      return;
    }

    choices.clear();
    for (auto& child : children) {
      if (IsConstructible(child)) {
        choices.push_back(child);
      }
    }
    result = State::AmbiguousConstructible;
    return;
  }

  // Just create the file.
  // If that goes well, a transaction will also occur.
  result = State::Constructible;
  process();
}

bool EditorImporter::process() {
  constexpr bool StayAlive = true;
  constexpr bool Die = false;

  if (result == State::Constructible) {
    auto object = SpawnState(data_id);
    if (object == nullptr) {
      result = State::NotConstructible;
      return Die;
    }
    fileState = std::unique_ptr<kpi::INode>{
        dynamic_cast<kpi::INode*>(object.release())};
    if (fileState == nullptr) {
      result = State::NotConstructible;
      return Die;
    }
    // auto fn = std::bind(&EditorImporter::messageHandler, std::ref(*this));
    auto fn = [](...) {};
    transaction.emplace(
        kpi::IOTransaction{*fileState.get(), provider->slice(), fn});

    // TODO: Move elsewhere..
    auto path = getPath();
    if (path.ends_with("dae") || path.ends_with("fbx") ||
        path.ends_with("obj") || path.ends_with("smd")) {
      path.resize(path.size() - 3);
      if (data_id.find("j3d") != std::string::npos)
        path += "bmd";
      else if (data_id.find("g3d") != std::string::npos)
        path += "brres";
      setPath(path);
    }
  }

  const auto msg = transact();
  switch (msg) {
  case kpi::TransactionState::Complete:
    result = State::Success;
    return Die;
  case kpi::TransactionState::ConfigureProperties:
    result = State::ConfigureProperties;
    return StayAlive;
  case kpi::TransactionState::ResolveDependencies:
    result = State::ResolveDependencies;
    return StayAlive;
  case kpi::TransactionState::Failure:
    result = State::InvalidData;
    return Die;
  }
  return StayAlive;
}

kpi::TransactionState EditorImporter::transact() {
  auto fn = [](...) {};
  transaction->callback = [&](kpi::IOMessageClass message_class,
                              const std::string_view domain,
                              const std::string_view message_body) {
    messageHandler(message_class, domain, message_body);
  };
  assert(transaction.has_value());
  mDeserializer->read_(*transaction);
  transaction->callback = fn;
  return transaction->state;
}

void EditorImporter::messageHandler(kpi::IOMessageClass message_class,
                                    const std::string_view domain,
                                    const std::string_view message_body) {
  mMessages.emplace_back(message_class, std::string(domain),
                         std::string(message_body));
}

} // namespace riistudio::frontend
