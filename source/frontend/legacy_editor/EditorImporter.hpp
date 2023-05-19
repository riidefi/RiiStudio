#pragma once

#include <LibBadUIFramework/Plugins.hpp> // kpi::IOMessageClass
#include <frontend/file_host.hpp>        // FileData
#include <frontend/widgets/ErrorDialogList.hpp>
#include <memory>              // std::unique_ptr
#include <optional>            // std::optional
#include <rsl/SmallVector.hpp> // rsl::small_vector
#include <string_view>         // std::string_view

namespace riistudio::frontend {

class EditorImporter {
public:
  enum class State : int {
    // Failure types
    _Failure,
    NotImportable = _Failure, //!< Binary file is unsupported
    NotConstructible,         //!< Cannot create or edit the file data
    InvalidData,              //!< Importer failed. Check the log for more info.
    Canceled,                 //!< To be set by children.
    _LastFailure = Canceled,

    // Progress types
    _Progress,
    AmbiguousConstructible = _Progress, //!< User must pick an option
    Constructible, //!< Ambiguity resolved, or there was never one to begin
                   //!< with. Create the file!
    ConfigureProperties, //!< Set importer requested properties
    ResolveDependencies, //!< Supply missing textures
    Communicating,       //!< Waiting on importer
                         //!< (only visible when multithreading)
    _LastProgress = Communicating,

    // Success
    Success
  };

  EditorImporter(FileData&& data, kpi::INode* fileState);

  State getState() const { return result; }
  bool failed() const {
    const int res = static_cast<int>(result);
    return res >= static_cast<int>(State::_Failure) &&
           res <= static_cast<int>(State::_LastFailure);
  }
  bool inProgress() const {
    const int res = static_cast<int>(result);
    return res >= static_cast<int>(State::_Progress) &&
           res <= static_cast<int>(State::_LastProgress);
  }
  bool succeeded() const { return result == State::Success; }
  const char* describeError() const {
    switch (result) {
    case State::NotImportable:
      return "Unknown file format."_j;
    case State::NotConstructible:
      return "The file format cannot be edited."_j;
    case State::InvalidData:
      return "File data is corrupted and cannot be read."_j;
    case State::Canceled:
      return "Operation was canceled by the user."_j;
    default:
      return "The importer is not in a failure state";
    }
  }
  std::unique_ptr<kpi::INode> takeResult() {
    if (result != State::Success)
      return nullptr;
    return std::move(fileState);
  }
  std::string getPath() const {
    if (!mPath.empty())
      return mPath;
    assert(provider != nullptr);
    if (provider == nullptr)
      return {};
    return std::string(provider->getFilePath());
  }
  void setPath(const std::string& path) { mPath = path; }

protected:
  // Return if the importer should stay alive
  // Only call when current request is done (properties are supplied,
  // dependencies are resolved)
  bool process();

  void importerRender() {
    if (mDeserializer)
      mDeserializer->render();
  }

  State result = State::NotImportable;
  std::optional<kpi::IOTransaction> transaction = std::nullopt;

  // question
  std::vector<std::string> choices;
  // answer
  std::string data_id;

private:
  kpi::TransactionState transact();

  // Record a message
  void messageHandler(kpi::IOMessageClass message_class,
                      const std::string_view domain,
                      const std::string_view message_body);

  std::unique_ptr<kpi::IBinaryDeserializer> mDeserializer = nullptr;
  std::unique_ptr<kpi::INode> fileState = nullptr;
  std::unique_ptr<oishii::DataProvider> provider;

  std::string mPath;

public:
  rsl::small_vector<Message, 16> mMessages;
};

} // namespace riistudio::frontend
