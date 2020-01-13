#include <RiiStudioCLI/core.hpp>
#include <LibCube/Export/Exports.hpp>
#include <fstream>
#include <oishii/reader/binary_reader.hxx>

namespace rs_cli {

System::System(int argc, char* const* argv)
{
	mPluginFactory.registerPlugin(libcube::Package());
	mPluginFactory.installModule("nw.dll");

	std::vector<std::string_view> args(argc);
	for (int i = 0; i < argc; ++i)
		args[i] = argv[i];

	mSession.populate(args);
	mSession.execute(*this);
}

System::~System()
{}


std::optional<System::SpawnedImporter> System::spawnImporter(const std::string& path)
{
	DebugReport("Path: %s\n", path.c_str());
	std::ifstream stream(path, std::ios::binary | std::ios::ate);

	if (!stream)
	{
		printf("Couldn't open file\n");
		return {};
	}
	const u32 size = stream.tellg();
	stream.seekg(0, std::ios::beg);
	DebugReport("Filesize: %u\n", size);
	std::vector<u8> data(size);
	if (!stream.read(reinterpret_cast<char*>(data.data()), size))
	{
		printf("Stream read failed.\n");
		return {};
	}
	auto reader = std::make_unique<oishii::BinaryReader>(std::move(data), size, path.c_str());
	auto pl_im = mPluginFactory.spawnImporter(path, *reader.get());
	return System::SpawnedImporter
	{
		std::move(pl_im->fileStateId),
		std::move(pl_im->importer),
		std::move(reader)
	};
}
//std::unique_ptr<pl::Exporter> System::spawnExporter(const std::string& id);
std::unique_ptr<pl::FileState> System::spawnFileState(const std::string& id)
{
	return mPluginFactory.spawnFileState(id);
}

} // namespace rs_cli
