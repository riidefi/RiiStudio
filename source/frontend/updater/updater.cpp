#include "updater.hpp"

#ifndef _WIN32
namespace riistudio {
Updater::Updater() = default;
Updater::~Updater() = default;
void Updater::draw() {}
class JSON {};
} // namespace riistudio
#else

#include <Windows.h>
// #include <securitybaseapi.h>

#include <Libloaderapi.h>
#include <core/util/gui.hpp>
#include <curl/curl.h>
#include <elzip/elzip.hpp>
#include <frontend/applet.hpp>
#include <frontend/widgets/changelog.hpp>
#include <io.h>
#include <nlohmann/json.hpp>


IMPORT_STD;
#include <iostream>

namespace riistudio {

class JSON {
public:
  nlohmann::json data;
};

const char* REPO_URL =
    "https://api.github.com/repos/riidefi/RiiStudio/releases/latest";

// From
// https://gist.github.com/alghanmi/c5d7b761b2c9ab199157#file-curl_example-cpp
static size_t WriteCallback(void* contents, size_t size, size_t nmemb,
                            void* userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

Updater::Updater() {
  if (!InitRepoJSON()) {
    fprintf(stderr, "Cannot connect to Github\n");
    return;
  }
  if (mJSON->data.contains("name") && mJSON->data["name"].is_string())
    mLatestVer = mJSON->data["name"].get<std::string>();

  if (mLatestVer.empty()) {
    fprintf(stderr, "There is no name field\n");
    return;
  }

  const auto current_exe = ExecutableFilename();
  if (current_exe.empty()) {
    fprintf(stderr, "Cannot get the name of the current .exe\n");
    return;
  }

  mShowUpdateDialog = GIT_TAG != mLatestVer;

  std::error_code ec;
  auto temp_dir = std::filesystem::temp_directory_path(ec);

  if (ec) {
    std::cout << ec.message() << std::endl;
    return;
  }

  const auto temp = temp_dir / "RiiStudio_temp.exe";

  if (std::filesystem::exists(temp)) {
    mShowChangelog = true;
    remove(temp);
  }
}

Updater::~Updater() {}

void Updater::draw() {
  if (mJSON == nullptr)
    return;

  if (mJSON->data.contains("body") && mJSON->data["body"].is_string())
    DrawChangeLog(&mShowChangelog, mJSON->data["body"].get<std::string>());

  // Hack.. wait one frame for the UI to size properly
  if (mForceUpdate && !mFirstFrame) {
    if (!InstallUpdate()) {
      // Give up.. admin perms didn't solve our problem
    }

    mForceUpdate = false;
  }

  if (!mLaunchPath.empty()) {
    printf("Launching update %s\n", mLaunchPath.c_str());
    LaunchUpdate(mLaunchPath);
    // (Never reached)
    mShowUpdateDialog = false;
  }

  if (!mShowUpdateDialog)
    return;

  const auto wflags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  ImGui::OpenPopup("RiiStudio Update"_j);
  if (ImGui::BeginPopupModal("RiiStudio Update"_j, nullptr, wflags)) {
    if (mIsInUpdate) {
      ImGui::Text("Installing Riistudio %s.."_j, mLatestVer.c_str());
      ImGui::ProgressBar(mUpdateProgress);
    } else {
      ImGui::Text("A new version of RiiStudio (%s) was found. Would you like "
                  "to update?"_j,
                  mLatestVer.c_str());
      if (ImGui::Button("Yes"_j, ImVec2(170, 0))) {
        if (!InstallUpdate() && mNeedAdmin) {
          RetryAsAdmin();
          // (Never reached)
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("No"_j, ImVec2(170, 0))) {
        mShowUpdateDialog = false;
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }
  mFirstFrame = false;
}

bool Updater::InitRepoJSON() {
  CURL* curl = curl_easy_init();

  if (curl == nullptr)
    return false;

  std::string rawJSON = "";

  curl_easy_setopt(curl, CURLOPT_URL, REPO_URL);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "RiiStudio");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawJSON);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    const char* str = curl_easy_strerror(res);
    printf("[libcurl] %s\n", str);
  }

  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return false;
  }

  mJSON = std::make_unique<JSON>();
  mJSON->data = nlohmann::json::parse(rawJSON);
  return true;
}

std::string Updater::ExecutableFilename() {
  std::array<char, 1024> pathBuffer{};
  const int n =
      GetModuleFileNameA(nullptr, pathBuffer.data(), pathBuffer.size());

  // We don't want a truncated path
  if (n < 1020)
    return std::string(pathBuffer.data(), n);
  return "";
}

static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}

static std::thread sThread;

bool Updater::InstallUpdate() {
  const auto current_exe = ExecutableFilename();
  if (current_exe.empty())
    return false;

  if (!(mJSON->data.contains("assets") && mJSON->data["assets"].size() > 0 &&
        mJSON->data["assets"][0].contains("browser_download_url")))
    return false;

  const auto folder = std::filesystem::path(current_exe).parent_path();
  const auto new_exe = folder / "RiiStudio.exe";
  const auto temp_exe =
      std::filesystem::temp_directory_path() / "RiiStudio_temp.exe";
  const auto download = folder / "download.zip";

  std::error_code error_code;
  std::filesystem::rename(current_exe, temp_exe, error_code);

  if (error_code) {
    std::cout << error_code.message() << std::endl;
    // Request admin perms...
    mNeedAdmin = true;
    return false;
  }

  const auto url =
      mJSON->data["assets"][0]["browser_download_url"].get<std::string>();

  auto progress_func =
      +[](void* userdata, double total, double current, double, double) {
        Updater* updater = reinterpret_cast<Updater*>(userdata);
        updater->SetProgress(current / total);
        return 0;
      };

  mIsInUpdate = true;
  sThread = std::thread(
      [=](Updater* updater) {
        CURL* curl = curl_easy_init();
        assert(curl);
        FILE* fp = fopen(download.string().c_str(), "wb");
        printf("%s\n", url.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "RiiStudio");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE);
        auto res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
          const char* str = curl_easy_strerror(res);
          printf("libcurl said %s\n", str);
        }

        curl_easy_cleanup(curl);
        fclose(fp);

        elz::extractZip(download.string(), folder.string());
        std::filesystem::remove(download);

        updater->QueueLaunch(new_exe.string());
      },
      this);
  return true;
}

void Updater::RetryAsAdmin() {
#ifdef _WIN32
  auto path = ExecutableFilename();
  ShellExecute(nullptr, "runas", path.c_str(), "--update", 0, SW_SHOWNORMAL);
#else
// FIXME: Provide Linux/Mac version
#endif

  exit(0);
}

void Updater::LaunchUpdate(const std::string& new_exe) {
  // On slow machines, the thread may not have been joined yet--so wait for it.
  sThread.join();

  assert(!sThread.joinable());

#ifdef _WIN32
  // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-processes

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  std::array<char, 32> buf{};

  CreateProcessA(new_exe.c_str(), // lpApplicationName
                 buf.data(),      // lpCommandLine
                 nullptr,         // lpProcessAttributes
                 nullptr,         // lpThreadAttributes
                 false,           // bInheritHandles
                 0,               // dwCreationFlags
                 nullptr,         // lpEnvironment
                 nullptr,         // lpCurrentDirectory
                 &si,             // lpStartupInfo
                 &pi              // lpProcessInformation
  );

  // TODO: If we were launched as an admin, drop those permissions
  // AdjustTokenPrivileges(nullptr, true, nullptr, 0, nullptr, 0);

#else
  // FIXME: Provide Linux/Mac version
#endif

  exit(0);
}

} // namespace riistudio

#endif
