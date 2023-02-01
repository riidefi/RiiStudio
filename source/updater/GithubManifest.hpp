#pragma once

#include <core/common.h>
#include <nlohmann/json.hpp>
#include <rsl/Download.hpp>

namespace riistudio {

class GithubManifest {
public:
  static Result<GithubManifest> fromString(std::string str) {
    auto j = nlohmann::json::parse(str, nullptr, false);
    if (j.is_discarded()) {
      return std::unexpected("Malformed JSON");
    }
    return GithubManifest{.json = j};
  }

  struct Asset {
    std::string browser_download_url;
  };

  std::vector<Asset> getAssets() const {
    std::vector<Asset> results;
    if (!json.contains("assets") || !json["assets"].is_array()) {
      return {};
    }

    for (const auto& asset : json["assets"]) {
      if (!asset.is_object()) {
        return {};
      }
      if (!asset.contains("browser_download_url")) {
        return {};
      }
      if (!asset["browser_download_url"].is_string()) {
        return {};
      }
      std::string url = asset["browser_download_url"].get<std::string>();
      Asset result{
          .browser_download_url = url,
      };
      results.push_back(result);
    }
    return results;
  }

  // Holds the release tag
  std::optional<std::string> name() const {
    if (json.contains("name") && json["name"].is_string())
      return json["name"].get<std::string>();
    return std::nullopt;
  }

  // Holds the description / changelog
  std::optional<std::string> body() const {
    if (json.contains("body") && json["body"].is_string())
      return json["body"].get<std::string>();
    return std::nullopt;
  }

  nlohmann::json json;
};

// Repo in the form of "riidefi/RiiStudio"
Result<GithubManifest> DownloadLatestRelease(std::string_view repo,
                                             std::string_view user_agent) {
  auto ok = rsl::DownloadString(
      std::format("https://api.github.com/repos/{}/releases/latest", repo),
      std::string(user_agent));
  if (!ok) {
    return std::unexpected(std::format("[rsl::DownloadString] {}", ok.error()));
  }
  std::string rawJSON = *ok;

  auto man = GithubManifest::fromString(rawJSON);
  if (!man) {
    return std::unexpected(std::format("[GithubManifest] {}", man.error()));
  }
  return man;
}

} // namespace riistudio
