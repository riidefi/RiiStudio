#include "IO.hpp"

#include <LibBadUIFramework/Plugins.hpp> // kpi::LightIOTransaction
#include <core/common.h>
#include <core/util/oishii.hpp>

// BRRES
#include <plugins/g3d/G3dIo.hpp>

// KMP
#include <librii/kmp/io/KMP.hpp>

IMPORT_STD;

namespace riistudio::lvl {

struct SimpleTransaction {
  SimpleTransaction() {
    trans.callback = [](kpi::IOMessageClass mc, std::string_view domain,
                        std::string_view msg) {
      rsl::warn("[{}] {}: {}", magic_enum::enum_name(mc), domain, msg);
    };
  }

  bool success() const {
    return trans.state == kpi::TransactionState::Complete;
  }

  kpi::LightIOTransaction trans;
};

Result<std::unique_ptr<g3d::Collection>> ReadBRRES(const std::vector<u8>& buf,
                                                   std::string path,
                                                   NeedResave need_resave) {
  auto result = std::make_unique<g3d::Collection>();

  SimpleTransaction trans;
  oishii::BinaryReader reader(buf, path, std::endian::big);
  g3d::ReadBRRES(*result, reader, trans.trans);

  // Tentatively allow previewing models we can't rebuild
  if (need_resave == NeedResave::AllowUnwritable &&
      trans.trans.state == kpi::TransactionState::FailureToSave)
    return result;

  if (!trans.success())
    return std::unexpected("Transaction failed");

  return result;
}

Result<std::unique_ptr<librii::kmp::CourseMap>>
ReadKMP(const std::vector<u8>& buf, std::string path) {
  auto map = TRY(librii::kmp::readKMP(buf));
  return std::make_unique<librii::kmp::CourseMap>(map);
}

std::vector<u8> WriteKMP(const librii::kmp::CourseMap& map) {
  oishii::Writer writer(std::endian::big);

  librii::kmp::writeKMP(map, writer);

  return writer.takeBuf();
}

Result<std::unique_ptr<librii::kcol::KCollisionData>>
ReadKCL(const std::vector<u8>& buf, std::string path) {
  auto result = std::make_unique<librii::kcol::KCollisionData>();

  auto res = librii::kcol::ReadKCollisionData(*result, buf, buf.size());

  {
    const auto metadata = librii::kcol::InspectKclFile(buf);
    std::cout << librii::kcol::GetKCLVersion(metadata) << std::endl;
  }

  if (!res.empty()) {
    return std::unexpected(res);
  }

  return result;
}

} // namespace riistudio::lvl
