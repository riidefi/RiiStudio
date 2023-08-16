#include "Download.hpp"

#ifdef LEGACY_CPP_IMPLEMENTATION
#include <curl/curl.h>
#endif

namespace rsl {

#ifndef LEGACY_CPP_IMPLEMENTATION
extern "C" {
char* download_string(const char* url, const char* user_agent, int* err);
void free_string(char* s);
char* download_file(const char* dest_path, const char* url,
                    const char* user_agent,
                    int (*progress_func)(void* userdata, double total,
                                         double current, double, double),
                    void* progress_data);
}

Result<std::string> DownloadString(std::string url, std::string user_agent) {
#ifdef __EMSCRIPTEN__
  return std::unexpected("WASM UNSUPPORTED");
#else
  int err = 0;
  char* c_str = download_string(url.c_str(), user_agent.c_str(), &err);

  fprintf(stderr, "RESULT: %s\n", c_str ? c_str : "???");

  if (c_str == nullptr)
    return std::unexpected("DownloadString failed with unspecified error");

  std::string result(c_str);
  free_string(c_str);

  if (err) {
    return std::unexpected("DownloadString failed: " + result);
  }
  return result;
#endif
}
void DownloadFile(std::string destPath, std::string url, std::string user_agent,
                  int (*progress_func)(void* userdata, double total,
                                       double current, double, double),
                  void* progress_data) {
#ifdef __EMSCRIPTEN__
#else
  char* result =
      download_file(destPath.c_str(), url.c_str(), user_agent.c_str(),
                    progress_func, progress_data);

  if (std::strcmp(result, "Success") != 0) {
    fprintf(stderr, "DownloadFile error: %s\n", result);
  }

  free_string(result);
#endif
}
#else
// From
// https://gist.github.com/alghanmi/c5d7b761b2c9ab199157#file-curl_example-cpp
static size_t WriteCallback(void* contents, size_t size, size_t nmemb,
                            void* userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}
Result<std::string> DownloadString(std::string url, std::string user_agent) {
  CURL* curl = curl_easy_init();
  if (curl == nullptr)
    return std::unexpected("Failed to initialize CURL");
  std::string rawJSON = "";

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawJSON);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    const char* str = curl_easy_strerror(res);
    return std::unexpected(str);
  }

  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return std::unexpected("Failed to de-initialize CURL");
  }

  return rawJSON;
}

static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}
void DownloadFile(std::string destPath, std::string url, std::string user_agent,
                  int (*progress_func)(void* userdata, double total,
                                       double current, double, double),
                  void* progress_data) {
  CURL* curl = curl_easy_init();
  assert(curl);
  FILE* fp = fopen(destPath.c_str(), "wb");
  rsl::trace("Downloading {}...", url);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
  curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
  curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  auto res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    const char* str = curl_easy_strerror(res);
    rsl::error("[libcurl] {}", str);
  }

  curl_easy_cleanup(curl);
  fclose(fp);
}
#endif

} // namespace rsl
