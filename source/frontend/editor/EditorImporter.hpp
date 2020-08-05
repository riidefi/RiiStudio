#pragma once

#include <core/kpi/Plugins.hpp>          // kpi::IOMessageClass
#include <frontend/file_host.hpp>        // FileData
#include <memory>                        // std::unique_ptr
#include <optional>                      // std::optional
#include <string_view>                   // std::string_view
#include <vendor/llvm/ADT/SmallVector.h> // llvm::SmallVector

namespace riistudio::frontend {

struct Message {
  kpi::IOMessageClass message_class;
  std::string domain;
  std::string message_body;

  Message(kpi::IOMessageClass mclass, std::string&& mdomain, std::string&& body)
      : message_class(mclass), domain(std::move(mdomain)),
        message_body(std::move(body)) {}
};

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
                                        //!< (Unimplemented for now)
    ConfigureProperties,                //!< Set importer requested properties
    ResolveDependencies,                //!< Supply missing textures
    Communicating,                      //!< Waiting on importer
                                        //!< (only visible when multithreading)
    _LastProgress = Communicating,

    // Success
    Success
  };

  EditorImporter(FileData&& data);

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
      return "Unknown file format.";
    case State::NotConstructible:
      return "The file format cannot be edited.";
    case State::InvalidData:
      return "File data is corrupted and cannot be read.";
    case State::Canceled:
      return "Operation was canceled by the user.";
    default:
      assert(!"The importer is not in a failure state");
      return nullptr;
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
      return nullptr;
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
  llvm::SmallVector<Message, 16> mMessages;
};

} // namespace riistudio::frontend
