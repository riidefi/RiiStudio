#include <core/util/oishii.hpp>
#include <librii/egg/BDOF.hpp>
#include <librii/egg/Blight.hpp>
#include <librii/egg/LTEX.hpp>
#include <librii/egg/PBLM.hpp>
#include <librii/kmp/io/KMP.hpp>
#include <plugins/api.hpp>
#include <rsl/Ranges.hpp>
#include <vendor/llvm/Support/InitLLVM.h>

IMPORT_STD;

bool gIsAdvancedMode = false;

namespace riistudio {
const char* translateString(std::string_view str) { return str.data(); }
} // namespace riistudio

namespace llvm {
int DisableABIBreakingChecks;
} // namespace llvm

void save(std::string_view path, kpi::INode& root,
          std::vector<u8>* data = nullptr, std::span<const u32> bps = {}) {
  printf("Writing to %s\n", std::string(path).c_str());
  oishii::Writer writer(0);

  if (data != nullptr) {
    writer.attachDataForMatchingOutput(*data);
  }
  for (u32 bp : bps) {
    writer.add_bp<u32>(bp);
  }

  auto ex = SpawnExporter(root);
  auto ok = ex->write_(root, writer);
  if (!ok) {
    fprintf(stderr, "Error writing file: %s\n", ok.error().c_str());
    return;
  }

  OishiiFlushWriter(writer, path);
}

std::optional<std::pair<std::unique_ptr<kpi::INode>, std::vector<u8>>>
open(std::string path, std::span<const u32> bps = {}) {
  auto file = OishiiReadFile(path);
  if (!file.has_value()) {
    std::cout << "Failed to read file!\n";
    return std::nullopt;
  }

  auto importer = SpawnImporter(std::string(path), file->slice());

  if (!importer.second) {
    printf("Cannot spawn importer..\n");
    return std::nullopt;
  }
  if (!IsConstructible(importer.first)) {
    printf("Non constructable state.. find parents\n");

    const auto children = GetChildrenOfType(importer.first);
    if (children.empty()) {
      printf("No children. Cannot construct.\n");
      return std::nullopt;
    }
    assert(/*children.size() == 1 &&*/ IsConstructible(children[0])); // TODO
    importer.first = children[0];
  }

  std::unique_ptr<kpi::INode> fileState{
      dynamic_cast<kpi::INode*>(SpawnState(importer.first).release())};
  if (!fileState.get()) {
    printf("Cannot spawn file state %s.\n", importer.first.c_str());
    return std::nullopt;
  }
  std::vector<std::string> logs;
  const auto add_log = [&logs](kpi::IOMessageClass message_class,
                               const std::string_view domain,
                               const std::string_view message_body) {
    logs.push_back(std::format("{}: {} (in {})\n",
                               magic_enum::enum_name(message_class),
                               message_body, domain));
  };
  kpi::IOTransaction transaction{{
                                     add_log,
                                     kpi::TransactionState::Complete,
                                 },
                                 *fileState,
                                 file->slice()};
  for (u32 bp : bps) {
    importer.second->addBp(bp);
  }
  do {
    if (transaction.state == kpi::TransactionState::ResolveDependencies) {
      fprintf(stderr, "Cannot resolve dependencies in test mode.\n");
      return std::nullopt;
    }
    if (transaction.state == kpi::TransactionState::ConfigureProperties) {
      transaction.state = kpi::TransactionState::Complete;
    }

    importer.second->read_(transaction);
  } while (transaction.state != kpi::TransactionState::Failure &&
           transaction.state != kpi::TransactionState::FailureToSave &&
           transaction.state != kpi::TransactionState::Complete);

  {
    for (auto& log : logs) {
      fprintf(stderr, "%s", log.c_str());
    }
  }

  return std::make_pair(std::move(fileState), file->slice() | rsl::ToList());
}

// XXX: Hack, though we'll refactor all of this way soon
extern std::string rebuild_dest;

void rebuild(std::string from, const std::string_view to, bool check,
             std::span<const s32> bps) {
  rebuild_dest = to;

  if (from.ends_with("kmp") || from.ends_with("blight") ||
      from.ends_with("blmap") || from.ends_with("bdof") ||
      from.ends_with("bblm")) {
    auto file = OishiiReadFile(from);
    if (!file.has_value()) {
      printf("Cannot rebuild\n");
      return;
    }

    oishii::Writer writer(0);
    for (auto bp : bps) {
      if (bp > 0) {
        writer.add_bp<u32>(bp);
      }
    }
    oishii::BinaryReader reader(file->slice());
    for (auto bp : bps) {
      if (bp < 0)
        reader.add_bp<u32>(-bp);
    }
    rsl::SafeReader safe(reader);
    if (from.ends_with("kmp")) {
      auto map = librii::kmp::readKMP(file->slice());
      if (!map) {
        fprintf(stderr, "Failed to read kmp: %s\n", map.error().c_str());
        return;
	  }
      printf("Writing to %s\n", std::string(to).c_str());
      librii::kmp::writeKMP(*map, writer);
    } else if (from.ends_with("blight")) {
      writer.attachDataForMatchingOutput(file->slice() | rsl::ToList());
      librii::egg::Blight lights;
      auto ok = lights.read(reader);
      if (!ok) {
        fprintf(stderr, "Err: %s", ok.error().c_str());
      }
      printf("Writing to %s\n", std::string(to).c_str());
      lights.save(writer);
    } else if (from.ends_with("blmap")) {
      writer.attachDataForMatchingOutput(file->slice() | rsl::ToList());
      librii::egg::LightMap lmap;
      lmap.read(safe);
      printf("Writing to %s\n", std::string(to).c_str());
      lmap.write(writer);
    } else if (from.ends_with("bdof")) {
      writer.attachDataForMatchingOutput(file->slice() | rsl::ToList());
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
      writer.attachDataForMatchingOutput(file->slice() | rsl::ToList());
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
    }
    OishiiFlushWriter(writer, to);
    return;
  }

  auto result =
      open(from, bps | std::views::filter([](auto& x) { return x < 0; }) |
                     std::views::transform([](auto x) -> u32 { return -x; }) |
                     rsl::ToList());
  if (!result) {
    printf("Cannot rebuild!\n");
    return;
  }
  auto& [data, raw] = *result;
  save(to, *data, check ? &raw : nullptr,
       bps | std::views::filter([](auto& x) { return x > 0; }) |
           std::views::transform([](auto x) -> u32 { return x; }) |
           rsl::ToList());
}

extern bool gTestMode;

#define ANNOUNCE(TITLE) printf("------\n" TITLE "\n\n")

int main(int argc, const char** argv) {
  gTestMode = true;

  ANNOUNCE("Initializing LLVM");
  llvm::InitLLVM init_llvm(argc, argv);

  ANNOUNCE("Initializing plugins");
  InitAPI();

  ANNOUNCE("Performing tasks");
  if (argc < 3) {
    fprintf(stderr,
            "Error: Too few arguments:\ntests.exe <from> <to> [check?]\n");
  } else {
    std::vector<s32> bps;
    for (int i = 4; i < argc; ++i) {
      bps.push_back(std::stoi(argv[i], nullptr, 16));
    }
    rebuild(argv[1], argv[2], argc > 3 && !strcmp(argv[3], "check"), bps);
  }

  ANNOUNCE("Done!");
  DeinitAPI();
}
