#include "updater.hpp"

#include <Windows.h>

#include <Libloaderapi.h>
#include <curl/curl.h>
#include <filesystem>
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")

#include <core/util/gui.hpp>
#include <elzip/elzip.hpp>
#include <frontend/applet.hpp>

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

  const auto wflags = ImGuiWindowFlags_NoResize; //| ImGuiWindowFlags_NoMove;

  ImGui::OpenPopup("RiiStudio Update");
  if (ImGui::BeginPopupModal("RiiStudio Update", nullptr, wflags)) {
    ImGui::Text("A new version of RiiStudio (%s) was found. Would you like "
                "to update?",
                mLatestVer.c_str());
    if (ImGui::Button("Yes", ImVec2(170, 0))) {
      InstallUpdate();
      mShowUpdateDialog = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("No", ImVec2(170, 0))) {
      mShowUpdateDialog = false;
      ImGui::CloseCurrentPopup();
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

  if (n < 1024)
    return std::string(pathBuffer.data(), n);
  return "";
}

void Updater::InstallUpdate() {
  const auto current_exe = ExecutableFilename();
  const auto url = ParseJSON("browser_download_url", "");

  if (current_exe.empty() || url.empty())
    return;

  const auto folder = std::filesystem::path(current_exe).parent_path();
  const auto new_exe = folder / "RiiStudio.exe";
  const auto temp_exe = folder / "temp.exe";
  const auto download = folder / "download.zip";

  URLDownloadToFile(nullptr, url.c_str(), download.string().c_str(), 0,
                    nullptr);

  std::filesystem::rename(current_exe, temp_exe);

  elz::extractZip(download.string(), folder.string());
  remove(download);

  // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-processes

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  std::array<char, 32> buf{};

  CreateProcessA(new_exe.string().c_str(), // lpApplicationName
                 buf.data(),       // lpCommandLine
                 nullptr,          // lpProcessAttributes
                 nullptr,          // lpThreadAttributes
                 false,            // bInheritHandles
                 0,                // dwCreationFlags
                 nullptr,          // lpEnvironment
                 nullptr,          // lpCurrentDirectory
                 &si,              // lpStartupInfo
                 &pi               // lpProcessInformation
  );

  exit(0);
}

}; // namespace riistudio
