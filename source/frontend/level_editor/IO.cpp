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

  kpi::LightIOTransaction trans;
};

Result<std::unique_ptr<g3d::Collection>> ReadBRRES(const std::vector<u8>& buf,
                                                   std::string path,
                                                   NeedResave need_resave) {
  auto result = std::make_unique<g3d::Collection>();

  SimpleTransaction trans;
  oishii::BinaryReader reader(buf, path, std::endian::big);
  TRY(g3d::ReadBRRES(*result, reader, trans.trans));

// Tentatively allow previewing models we can't rebuild
#if 0
  if (need_resave == NeedResave::AllowUnwritable &&
      trans.trans.state == kpi::TransactionState::FailureToSave)
    return result;
#endif

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
  auto ok = librii::kcol::ReadKCollisionData(buf, buf.size());

  {
    const auto metadata = librii::kcol::InspectKclFile(buf);
    std::cout << librii::kcol::GetKCLVersion(metadata) << std::endl;
  }

  if (!ok) {
    return std::unexpected(ok.error());
  }

  return std::make_unique<librii::kcol::KCollisionData>(*ok);
}

} // namespace riistudio::lvl
