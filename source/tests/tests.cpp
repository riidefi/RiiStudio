#include <core/api.hpp>
#include <core/util/oishii.hpp>
#include <librii/egg/Blight.hpp>
#include <librii/kmp/io/KMP.hpp>
#include <rsl/Ranges.hpp>
#include <vendor/llvm/Support/InitLLVM.h>

import std.core;

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
  ex->write_(root, writer);

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
  kpi::IOTransaction transaction{{
                                     [](...) {},
                                     kpi::TransactionState::Complete,
                                 },
                                 *fileState,
                                 file->slice()};
  for (u32 bp : bps) {
    importer.second->addBp(bp);
  }
  importer.second->read_(transaction);

  return std::make_pair(std::move(fileState), file->slice() | rsl::ToList());
}

// XXX: Hack, though we'll refactor all of this way soon
extern std::string rebuild_dest;

void rebuild(std::string from, const std::string_view to, bool check,
             std::span<const s32> bps) {
  rebuild_dest = to;

  if (from.ends_with("kmp") || from.ends_with("blight")) {
    auto file = OishiiReadFile(from);
    if (!file.has_value()) {
      printf("Cannot rebuild\n");
      return;
    }

    oishii::Writer writer(0);
    if (from.ends_with("kmp")) {
      librii::kmp::CourseMap map;
      librii::kmp::readKMP(map, file->slice());
      printf("Writing to %s\n", std::string(to).c_str());
      librii::kmp::writeKMP(map, writer);
    } else if (from.ends_with("blight")) {
      writer.attachDataForMatchingOutput(file->slice() | rsl::ToList());
      oishii::BinaryReader reader(file->slice());
      librii::egg::Blight lights(reader);
      printf("Writing to %s\n", std::string(to).c_str());
      lights.save(writer);
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
