#include "updater.hpp"

#ifndef _WIN32
namespace riistudio {
Updater::Updater() = default;
void Updater::draw() {}
} // namespace riistudio
#else

#include <Windows.h>

#include <Libloaderapi.h>
#include <core/util/gui.hpp>
#include <curl/curl.h>
#include <elzip/elzip.hpp>
#include <filesystem>
#include <frontend/applet.hpp>
#include <thread>

namespace riistudio {

const char* VERSION = "Alpha 4.0";
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
  InitRepoJSON();
  mLatestVer = ParseJSON("name", VERSION);
  mShowUpdateDialog = VERSION != mLatestVer;

  const auto current_exe = ExecutableFilename();
  if (current_exe.empty())
    return;

  const auto folder = std::filesystem::path(current_exe).parent_path();
  const auto temp = folder / "temp.exe";

  if (std::filesystem::exists(temp))
    remove(temp);
}

void Updater::draw() {
  if (!mShowUpdateDialog)
    return;

  const auto wflags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  ImGui::OpenPopup("RiiStudio Update");
  if (ImGui::BeginPopupModal("RiiStudio Update", nullptr, wflags)) {
    if (mIsInUpdate) {
      ImGui::ProgressBar(mUpdateProgress);
      if (!mLaunchPath.empty()) {
        LaunchUpdate(mLaunchPath);
        // (Never reached)
        mShowUpdateDialog = false;
        ImGui::CloseCurrentPopup();
      }
    } else {
      ImGui::Text("A new version of RiiStudio (%s) was found. Would you like "
                  "to update?",
                  mLatestVer.c_str());
      if (ImGui::Button("Yes", ImVec2(170, 0))) {
        InstallUpdate();
      }
      ImGui::SameLine();
      if (ImGui::Button("No", ImVec2(170, 0))) {
        mShowUpdateDialog = false;
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }
}

void Updater::InitRepoJSON() {
  CURL* curl = curl_easy_init();

  if (curl == nullptr)
    return;

  curl_easy_setopt(curl, CURLOPT_URL, REPO_URL);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "RiiStudio");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mJSON);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    const char* str = curl_easy_strerror(res);
    printf("libcurl said %s\n", str);
  }

  curl_easy_cleanup(curl);
}

std::string Updater::ParseJSON(std::string key, std::string defaultValue) {
  int start = mJSON.find("\"" + key + "\"");
  if (start == std::string::npos)
    return defaultValue;

  start += key.size() + 4;
  int len = mJSON.find("\"", start) - start;

  return mJSON.substr(start, len);
}

std::string Updater::ExecutableFilename() {
  std::array<char, 1024> pathBuffer;
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

void Updater::InstallUpdate() {
  const auto current_exe = ExecutableFilename();
  if (current_exe.empty())
    return;

  const auto url = ParseJSON("browser_download_url", "");
  if (url.empty())
    return;

  auto progress_func =
      +[](void* userdata, double total, double current, double, double) {
        Updater* updater = reinterpret_cast<Updater*>(userdata);
        updater->SetProgress(current / total);
        return 0;
      };

  mIsInUpdate = true;
  static std::thread sThread = std::thread(
      [=](Updater* updater) {
        const auto folder = std::filesystem::path(current_exe).parent_path();
        const auto new_exe = folder / "RiiStudio.exe";
        const auto temp_exe = folder / "temp.exe";
        const auto download = folder / "download.zip";

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

        std::filesystem::rename(current_exe, temp_exe);

        elz::extractZip(download.string(), folder.string());
        remove(download);

        updater->QueueLaunch(new_exe.string());
      },
      this);
}

void Updater::LaunchUpdate(const std::string& new_exe) {
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
#else
  // FIXME: Provide Linux/Mac version
#endif

  exit(0);
}

} // namespace riistudio

#endif
