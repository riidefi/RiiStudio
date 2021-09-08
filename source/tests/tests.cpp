#include <core/api.hpp>
#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <plate/Platform.hpp>
#include <string>
#include <vendor/llvm/Support/InitLLVM.h>

bool gIsAdvancedMode = false;

namespace riistudio {
const char* translateString(std::string_view str) { return str.data(); }
} // namespace riistudio

namespace llvm {
int DisableABIBreakingChecks;
} // namespace llvm

void save(const std::string_view path, kpi::INode& root) {
  printf("Writing to %s\n", std::string(path).c_str());
  oishii::Writer writer(1024);

  auto ex = SpawnExporter(root);
  ex->write_(root, writer);

  plate::Platform::writeFile({writer.getDataBlockStart(), writer.getBufSize()},
                             path);
}

std::unique_ptr<kpi::INode> open(const std::string_view path) {
  std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
  std::vector<u8> vec(file.tellg());
  file.seekg(0, std::ios::beg);

  if (!file.read(reinterpret_cast<char*>(vec.data()), vec.size())) {
    std::cout << "Failed to read file!\n";
    return nullptr;
  }

  oishii::DataProvider provider(std::move(vec), path);
  oishii::BinaryReader reader(provider.slice());
  auto importer = SpawnImporter(std::string(path), provider.slice());

  if (!importer.second) {
    printf("Cannot spawn importer..\n");
    return nullptr;
  }
  if (!IsConstructible(importer.first)) {
    printf("Non constructable state.. find parents\n");

    const auto children = GetChildrenOfType(importer.first);
    if (children.empty()) {
      printf("No children. Cannot construct.\n");
      return nullptr;
    }
    assert(/*children.size() == 1 &&*/ IsConstructible(children[0])); // TODO
    importer.first = children[0];
  }

  std::unique_ptr<kpi::INode> fileState{
      dynamic_cast<kpi::INode*>(SpawnState(importer.first).release())};
  if (!fileState.get()) {
    printf("Cannot spawn file state %s.\n", importer.first.c_str());
    return nullptr;
  }
  kpi::IOTransaction transaction{{
                                     [](...) {},
                                     kpi::TransactionState::Complete,
                                 },
                                 *fileState,
                                 provider.slice()};
  importer.second->read_(transaction);

  return fileState;
}

// XXX: Hack, though we'll refactor all of this way soon
extern std::string rebuild_dest;

void rebuild(const std::string_view from, const std::string_view to) {
  rebuild_dest = to;

  auto data = open(from);
  if (!data) {
    printf("Cannot rebuild!\n");
    return;
  }
  save(to, *data);
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
    fprintf(stderr, "Error: Too few arguments:\ntests.exe <from> <to>\n");
  } else {
    rebuild(argv[1], argv[2]);
  }

  ANNOUNCE("Done!");
  DeinitAPI();
}
