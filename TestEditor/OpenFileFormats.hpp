#pragma once
// Included for pfd:: usage
#include <ThirdParty/FileDialogues.hpp>

// Pikmin 1 formats
#include <LibCube/Pikmin1/MOD/MOD.hpp> // Model format
#include <LibCube/Pikmin1/DCA/DCA.hpp> // Animation format
#include <LibCube/Pikmin1/DCK/DCK.hpp> // Animation format
#include <LibCube/Pikmin1/TXE/TXE.hpp> // P1 Texture format

// J3D formats
#include <LibCube/JSystem/J3D/Binary/BTI/BTI.hpp> // J3D Texture format

#include <fstream>

namespace libcube {
namespace pk1 = libcube::pikmin1;

template <class T>
inline std::unique_ptr<T> openParseFileFormat(const std::string& filterContent, const std::string& extension) {
	auto selection = pfd::open_file("Select a file", ".", { filterContent, extension }, false).result();
	if (selection.empty()) // user has pressed cancel
		return nullptr;

	std::string fileName(selection[0]);
	DebugReport("Opening file %s\n", fileName.c_str());

	std::ifstream fStream(fileName, std::ios::binary | std::ios::ate);

	if (!fStream.is_open())
		return nullptr;

	u32 size = static_cast<u32>(fStream.tellg());
	fStream.seekg(0, std::ios::beg);

	auto data = std::vector<u8>(size);
	if (fStream.read(reinterpret_cast<char*>(data.data()), size))
	{
		oishii::BinaryReader reader(std::move(data), size, fileName.c_str());
		reader.seekSet(0);

		std::unique_ptr<T> instantiated = std::make_unique<T>();
		reader.dispatch<T, oishii::Direct, false>(*instantiated.get());
		return instantiated;
	}

	fStream.close();
	return nullptr;
}

// Pikmin 1
static inline void openMODFile()
{
	auto format = openParseFileFormat<pk1::MOD>("Pikmin 1 Model Files (*.mod)", "*.mod");
	//  check if nullptr or not
}
static inline void openTXEFile()
{
	auto format = openParseFileFormat<pk1::TXE>("Pikmin 1 Texture Files (*.txe)", "*.txe");
}
static inline void openDCAFile()
{
	auto format = openParseFileFormat<pk1::DCA>("Pikmin 1 Animation Files (*.dca)", "*.dca");
}
static inline void openDCKFile()
{
	auto format = openParseFileFormat<pk1::DCK>("Pikmin 1 Animation Files (*.dck)", "*.dck");
}

// J3D
static inline void openBTIFile()
{
	auto format = openParseFileFormat<j3d::BTI>("J3D Texture Files (*.bti)", "*.bti");
}

// Other

}
