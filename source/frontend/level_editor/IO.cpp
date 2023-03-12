#include "IO.hpp"

#include <core/common.h>
#include <LibBadUIFramework/Plugins.hpp> // kpi::LightIOTransaction
#include <core/util/oishii.hpp>

// BRRES
#include <plugins/g3d/G3dIo.hpp>

// KMP
#include <librii/kmp/io/KMP.hpp>

IMPORT_STD;

namespace riistudio::lvl {

struct Reader {
  oishii::DataProvider mData;
  oishii::BinaryReader mReader;

  Reader(std::string path, const std::vector<u8>& data)
      : mData(OishiiReadFile(path, data.data(), data.size())),
        mReader(mData.slice()) {}
};

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
  Reader reader(path, buf);
  g3d::ReadBRRES(*result, reader.mReader, trans.trans);

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
  Reader reader(path, buf);
  auto map = librii::kmp::readKMP(reader.mData.slice());
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

  Reader reader(path, buf);
  auto res = librii::kcol::ReadKCollisionData(
      *result, reader.mData.slice(), reader.mData.slice().size_bytes());

  {
    const auto metadata = librii::kcol::InspectKclFile(reader.mData.slice());
    std::cout << librii::kcol::GetKCLVersion(metadata) << std::endl;
  }

  if (!res.empty()) {
    rsl::error("Error: {}", res.c_str());
    return nullptr;
  }

  return result;
}

} // namespace riistudio::lvl
