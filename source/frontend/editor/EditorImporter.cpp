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
  auto [data_id, importer] = SpawnImporter(data.mPath, provider->slice());

  if (!importer) {
    result = State::NotImportable;
    return;
  }

  if (!IsConstructible(data_id)) {
    printf("Non constructible state.. find parents\n");

    const auto children = GetChildrenOfType(data_id);
    if (children.empty()) {
      result = State::NotConstructible;
      return;
    }
    data_id = typeid(j3d::Collection).name();
    // assert(/*children.size() == 1 &&*/ IsConstructible(children[1])); // TODO
    // data_id = children[1];
  }

  auto object = SpawnState(data_id);
  if (object == nullptr) {
    result = State::NotConstructible;
    return;
  }
  fileState =
      std::unique_ptr<kpi::INode>{dynamic_cast<kpi::INode*>(object.release())};
  if (fileState == nullptr) {
    result = State::NotConstructible;
    return;
  }
  // auto fn = std::bind(&EditorImporter::messageHandler, std::ref(*this));
  auto fn = [](...) {};
  transaction.emplace(
      kpi::IOTransaction{*fileState.get(), provider->slice(), fn});

  mDeserializer = std::move(importer);
  // TODO: Move elsewhere..
  auto path = getPath();
  if (path.ends_with("dae") || path.ends_with("fbx") || path.ends_with("obj") ||
      path.ends_with("smd")) {
    path.resize(path.size() - 3);
    path += "bmd";
    setPath(path);
  }

  // Process one time first..
  process();
}

bool EditorImporter::process() {
  constexpr bool StayAlive = true;
  constexpr bool Die = false;

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
