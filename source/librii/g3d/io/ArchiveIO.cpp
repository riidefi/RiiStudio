#include "ArchiveIO.hpp"

#include "CommonIO.hpp"
#include <librii/g3d/data/Archive.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>

#include <rsl/Expected.hpp>
#define BRRES_SYS_NO_EXPECTED
#include <brres/lib/brres-sys/include/brres_sys.h>
#include <rsl/WriteFile.hpp>

#include <vendor/nlohmann/json.hpp>

namespace librii::g3d {
void TestJson(const librii::g3d::Archive& archive);

struct DumpResult {
  std::string jsonData;
  std::vector<u8> collatedBuffer;
};

DumpResult DumpJson(const librii::g3d::Archive& archive);
Result<librii::g3d::Archive> ReadJsonArc(std::string_view json,
                                         std::span<const u8> buffer);
} // namespace librii::g3d

namespace librii::g3d {

Archive::Archive() __attribute__((weak)) = default;
Archive::Archive(const Archive&) __attribute__((weak)) = default;
Archive::Archive(Archive&&) noexcept __attribute__((weak)) = default;
Archive::~Archive() __attribute__((weak)) = default;

Result<void> Archive::write(oishii::Writer& writer) const {
  auto buf = TRY(write());
  for (auto u : buf) {
    writer.write(u);
  }
  return {};
}
Result<void> Archive::write(std::string_view path) const {
  auto buf = TRY(write());
  TRY(rsl::WriteFile(buf, path));
  return {};
}
Result<std::vector<u8>> Archive::write() const {
  auto d = DumpJson(*this);
  auto buf = TRY(brres::write_bytes(d.jsonData, d.collatedBuffer));
  return buf;
}

Result<Archive> Archive::fromFile(std::string path,
                                  kpi::LightIOTransaction& transaction) {
  auto reader = TRY(oishii::BinaryReader::FromFilePath(path, std::endian::big));
  return fromMemory(reader.mBuf, path, transaction);
}
Result<Archive> Archive::fromFile(std::string path) {
  kpi::LightIOTransaction trans;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    rsl::error(msg);
  };
  return fromFile(path, trans);
}
Result<Archive> Archive::read(oishii::BinaryReader& reader,
                              kpi::LightIOTransaction& transaction) {
  return fromMemory(reader.mBuf, reader.getFile(), transaction);
}
Result<Archive> Archive::fromMemory(std::span<const u8> buf, std::string path,
                                    kpi::LightIOTransaction& trans) {
  auto res = TRY(brres::read_from_bytes(buf));
  auto arc = TRY(ReadJsonArc(res.jsonData, res.collatedBuffer));
  auto j = nlohmann::json::parse(res.jsonData);
  if (j.contains("warnings") && j["warnings"].is_array()) {
    for (auto& w : j["warnings"]) {
      if (!w.contains("mclass") || !w["mclass"].is_number_unsigned()) {
        continue;
	  }
      if (!w.contains("domain") || !w["domain"].is_string()) {
        continue;
      }
      if (!w.contains("body") || !w["body"].is_string()) {
        continue;
      }
      int mclass = w["mclass"];
      std::string domain = w["domain"];
      std::string body = w["body"];
      trans.callback(static_cast<kpi::IOMessageClass>(mclass), domain, body);
    }
  }
  return arc;
}
Result<Archive> Archive::fromMemory(std::span<const u8> buf, std::string path) {
  auto res = TRY(brres::read_from_bytes(buf));
  auto arc = TRY(ReadJsonArc(res.jsonData, res.collatedBuffer));
  return arc;
}

} // namespace librii::g3d
