#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Plugins.hpp>
#include <librii/rhst/RHST.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <string>

namespace riistudio::rhst {

struct RHSTReader {
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const;
  void read(kpi::IOTransaction& transaction);
};

kpi::Register<RHSTReader, kpi::Reader> RHSTInstaller;

std::string RHSTReader::canRead(const std::string& file,
                                oishii::BinaryReader& reader) const {
  if (reader.read<u32, oishii::EndianSelect::Big>() != 'RHST')
    return "";

  if (reader.read<u32, oishii::EndianSelect::Little>() != 1)
    return "";

  return typeid(lib3d::Scene).name();
}

void RHSTReader::read(kpi::IOTransaction& transaction) {
  std::string error_msg;
  auto result = librii::rhst::ReadSceneTree(transaction.data, error_msg);

  (void)result;

  transaction.state = result.has_value() ? kpi::TransactionState::Complete
                                         : kpi::TransactionState::Failure;
}

} // namespace riistudio::rhst
