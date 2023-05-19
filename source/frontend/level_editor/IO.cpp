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
    trans.callback = [](...) {};
  }

  bool success() const {
    return trans.state == kpi::TransactionState::Complete;
  }

  kpi::LightIOTransaction trans;
};

std::unique_ptr<g3d::Collection> ReadBRRES(const std::vector<u8>& buf,
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
    return nullptr;

  return result;
}

std::unique_ptr<librii::kmp::CourseMap> ReadKMP(const std::vector<u8>& buf,
                                                std::string path) {
  auto map = librii::kmp::readKMP(buf);
  if (!map) {
    return nullptr;
  }
  return std::make_unique<librii::kmp::CourseMap>(*map);
}

std::vector<u8> WriteKMP(const librii::kmp::CourseMap& map) {
  oishii::Writer writer(0);

  librii::kmp::writeKMP(map, writer);

  return writer.takeBuf();
}

std::unique_ptr<librii::kcol::KCollisionData>
ReadKCL(const std::vector<u8>& buf, std::string path) {
  auto result = std::make_unique<librii::kcol::KCollisionData>();

  auto res = librii::kcol::ReadKCollisionData(*result, buf, buf.size());

  {
    const auto metadata = librii::kcol::InspectKclFile(buf);
    std::cout << librii::kcol::GetKCLVersion(metadata) << std::endl;
  }

  if (!res.empty()) {
    rsl::error("Error: {}", res.c_str());
    return nullptr;
  }

  return result;
}

} // namespace riistudio::lvl
