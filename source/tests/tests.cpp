#include <core/util/oishii.hpp>
#include <librii/egg/BDOF.hpp>
#include <librii/egg/Blight.hpp>
#include <librii/egg/LTEX.hpp>
#include <librii/egg/PBLM.hpp>
#include <librii/kmp/io/KMP.hpp>
#include <librii/rarc/RARC.hpp>
#include <librii/szs/SZS.hpp>
#include <librii/u8/U8.hpp>
#include <plugins/g3d/G3dIo.hpp>
#include <plugins/j3d/J3dIo.hpp>
#include <rsl/InitLLVM.hpp>
#include <rsl/Ranges.hpp>

IMPORT_STD;

bool gIsAdvancedMode = false;

namespace riistudio {
const char* translateString(std::string_view str) { return str.data(); }
} // namespace riistudio

namespace llvm {
int DisableABIBreakingChecks;
} // namespace llvm

// XXX: Hack, though we'll refactor all of this way soon
extern std::string rebuild_dest;

void rebuild(std::string from, const std::string_view to, bool check,
             std::span<const s32> bps) {
  rebuild_dest = to;

  if (from.ends_with("kmp") || from.ends_with("blight") ||
      from.ends_with("blmap") || from.ends_with("bdof") ||
      from.ends_with("bblm") || from.ends_with("bmd") ||
      from.ends_with("bdl") || from.ends_with("brres") ||
      from.ends_with("bblm") || from.ends_with("szs") ||
      from.ends_with("arc") || from.ends_with("carc") || from.ends_with("u8")) {
    auto file = OishiiReadFile2(from);
    if (!file.has_value()) {
      printf("Cannot rebuild\n");
      return;
    }

    oishii::Writer writer(std::endian::big);
    for (auto bp : bps) {
      if (bp > 0) {
        writer.add_bp<u32>(bp);
      }
    }
    oishii::BinaryReader reader(*file, from, std::endian::big);
    for (auto bp : bps) {
      if (bp < 0)
        reader.add_bp<u32>(-bp);
    }
    rsl::SafeReader safe(reader);
    if (from.ends_with("kmp")) {
      auto map = librii::kmp::readKMP(*file);
      if (!map) {
        fprintf(stderr, "Failed to read kmp: %s\n", map.error().c_str());
        return;
      }
      printf("Writing to %s\n", std::string(to).c_str());
      librii::kmp::writeKMP(*map, writer);
    } else if (from.ends_with("blight")) {
      writer.attachDataForMatchingOutput(*file | rsl::ToList());
      librii::egg::Blight lights;
      auto ok = lights.read(reader);
      if (!ok) {
        fprintf(stderr, "Err: %s", ok.error().c_str());
      }
      printf("Writing to %s\n", std::string(to).c_str());
      lights.save(writer);
    } else if (from.ends_with("blmap")) {
      writer.attachDataForMatchingOutput(*file | rsl::ToList());
      librii::egg::LightMap lmap;
      lmap.read(safe);
      printf("Writing to %s\n", std::string(to).c_str());
      lmap.write(writer);
    } else if (from.ends_with("bdof")) {
      writer.attachDataForMatchingOutput(*file | rsl::ToList());
      auto bdof = librii::egg::bin::BDOF_Read(safe);
      if (!bdof) {
        fprintf(stderr, "Failed to read bdof: %s\n", bdof.error().c_str());
        return;
      }
      auto dof = librii::egg::From_BDOF(*bdof);
      if (!dof) {
        fprintf(stderr, "Failed to read bdof: %s\n", dof.error().c_str());
        return;
      }
      auto bdof2 = librii::egg::To_BDOF(*dof);
      printf("Writing to %s\n", std::string(to).c_str());
      librii::egg::bin::BDOF_Write(writer, bdof2);
    } else if (from.ends_with("bblm")) {
      writer.attachDataForMatchingOutput(*file | rsl::ToList());
      auto bdof = librii::egg::PBLM_Read(safe);
      if (!bdof) {
        fprintf(stderr, "Failed to read bblm: %s\n", bdof.error().c_str());
        return;
      }
      auto dof = librii::egg::From_PBLM(*bdof);
      if (!dof) {
        fprintf(stderr, "Failed to read bblm: %s\n", dof.error().c_str());
        return;
      }
      auto bdof2 = librii::egg::To_PBLM(*dof);
      printf("Writing to %s\n", std::string(to).c_str());
      librii::egg::PBLM_Write(writer, bdof2);
    } else if (from.ends_with("brres")) {
      // writer.attachDataForMatchingOutput(*file | rsl::ToList());
      riistudio::g3d::Collection brres;
      kpi::LightIOTransaction trans;
      trans.callback = [&](kpi::IOMessageClass message_class,
                           const std::string_view domain,
                           const std::string_view message_body) {
        auto msg =
            std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                        domain, message_body);
        rsl::error(msg);
      };
      riistudio::g3d::ReadBRRES(brres, reader, trans);
      if (trans.state != kpi::TransactionState::Complete) {
        fprintf(stderr, "Failed to read BRRES\n");
        return;
      }
      printf("Writing to %s\n", std::string(to).c_str());
      auto bruh = riistudio::g3d::WriteBRRES(brres, writer);
      if (!bruh) {
        fprintf(stderr, "Failed to write BRRES: %s\n", bruh.error().c_str());
        return;
      }
    } else if (from.ends_with("bmd") || from.ends_with("bdl")) {
      // writer.attachDataForMatchingOutput(*file | rsl::ToList());
      riistudio::j3d::Collection bmd;
      kpi::LightIOTransaction trans;
      trans.callback = [&](kpi::IOMessageClass message_class,
                           const std::string_view domain,
                           const std::string_view message_body) {
        auto msg =
            std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                        domain, message_body);
        rsl::error(msg);
      };

      if (auto ok = riistudio::j3d::ReadBMD(bmd, reader, trans); !ok) {
        fprintf(stderr, "Failed to read BMD/BDL: %s\n", ok.error().c_str());
        return;
      }
      printf("Writing to %s\n", std::string(to).c_str());
      auto bruh = riistudio::j3d::WriteBMD(bmd, writer);
      if (!bruh) {
        fprintf(stderr, "Failed to write BMD/BDL: %s\n", bruh.error().c_str());
        return;
      }
    } else if (from.ends_with("szs") || from.ends_with("arc") ||
               from.ends_with("carc") || from.ends_with("u8")) {
      const rsl::byte_view data_view = safe.slice();

      std::vector<u8> data;
      if (librii::szs::isDataYaz0Compressed(data_view)) {
        auto result = librii::szs::getExpandedSize(data_view);
        if (!result) {
          fprintf(stderr, "Failed to read szs: %s\n", result.error().c_str());
          return;
        }
        data.resize(*result);
        librii::szs::decode(data, data_view);
      } else {
        data.insert(data.begin(), data_view.begin(), data_view.end());
      }

      if (librii::RARC::IsDataResourceArchive(data)) {
        auto rarc = librii::RARC::LoadResourceArchive(data);
        if (!rarc) {
          fprintf(stderr, "Failed to read rarc: %s\n", rarc.error().c_str());
          return;
        }
        printf("Writing to %s\n", std::string(to).c_str());
        auto barc = librii::RARC::SaveResourceArchive(*rarc);
        if (!barc) {
          fprintf(stderr, "Failed to save rarc: %s\n", barc.error().c_str());
          return;
        }
        for (auto& b : *barc) {
          writer.write(b);
        }
      } else if (librii::U8::IsDataU8Archive(data)) {
        auto u8 = librii::U8::LoadU8Archive(data);
        if (!u8) {
          fprintf(stderr, "Failed to read u8: %s\n", u8.error().c_str());
          return;
        }
        printf("Writing to %s\n", std::string(to).c_str());
        for (auto& b : librii::U8::SaveU8Archive(*u8)) {
          writer.write(b);
        }
      } else {
        fprintf(stderr, "Unrecognized archive format; Failed to rebuild\n");
        return;
      }
    }
    writer.saveToDisk(to);
    return;
  }
  fprintf(stderr, "Unrecognized format; Failed to rebuild\n");
  return;
}

extern bool gTestMode;

#define ANNOUNCE(TITLE) printf("------\n" TITLE "\n\n")

int main(int argc, const char** argv) {
  gTestMode = true;

  ANNOUNCE("Initializing LLVM");
  rsl::InitLLVM init_llvm(argc, argv);

  ANNOUNCE("Performing tasks");
  if (argc < 3) {
    fprintf(stderr,
            "Error: Too few arguments:\ntests.exe <from> <to> [check?]\n");
  } else {
    std::vector<s32> bps;
    for (int i = 4; i < argc; ++i) {
      bps.push_back(std::stoi(argv[i], nullptr, 16));
    }
    bool check = argc > 3 && !strcmp(argv[3], "check");
    rebuild(argv[1], argv[2], check, bps);
  }

  ANNOUNCE("Done!");
}
