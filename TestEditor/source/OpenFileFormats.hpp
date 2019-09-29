#pragma once
// Included for pfd:: usage
#include <FileDialogues.hpp>

// Pikmin 1 formats
#include "SysDolphin/MOD/MOD.hpp" // Model format
#include "SysDolphin/DCA/DCA.hpp" // Animation format
#include "SysDolphin/DCK/DCK.hpp" // Animation format
#include "SysDolphin/TXE/TXE.hpp" // P1 Texture format

// J3D formats
#include "JSystem/J3D/BTI/BTI.hpp" // J3D Texture format

#include <fstream>

namespace libcube {
namespace pk1 = libcube::pikmin1;

// Pikmin 1
static void openMODFile()
{
	auto selection = pfd::open_file("Select a file", ".", { "Pikmin 1 Model Files (*.mod)", "*.mod" }, false).result();
	if (selection.empty()) // user has pressed cancel
		return;

	std::string fileName(selection[0]);
	DebugReport("Opening file %s\n", fileName.c_str());

	std::ifstream fStream(fileName, std::ios::binary | std::ios::ate);

	if (!fStream.is_open())
		return;

	u32 size = static_cast<u32>(fStream.tellg());
	fStream.seekg(0, std::ios::beg);

	auto data = std::unique_ptr<char>(new char[size]);
	if (fStream.read(data.get(), size))
	{
		oishii::BinaryReader reader(std::move(data), size, fileName.c_str());
		reader.seekSet(0); // reset offset inside file for reading (isn't inside bundle so this is OK)

		pk1::MOD modelFile;
		modelFile.parse(reader);
	}

	fStream.close();
	return;
}
static void openTXEFile()
{
	auto selection = pfd::open_file("Select a file", ".", { "Pikmin 1 Texture Files (*.txe)", "*.txe" }, false).result();
	if (selection.empty()) // user has pressed cancel
		return;

	std::string fileName(selection[0]);
	DebugReport("Opening file %s\n", fileName.c_str());

	std::ifstream fStream(fileName, std::ios::binary | std::ios::ate);

	if (!fStream.is_open())
		return;

	u32 size = static_cast<u32>(fStream.tellg());
	fStream.seekg(0, std::ios::beg);

	auto data = std::unique_ptr<char>(new char[size]);
	if (fStream.read(data.get(), size))
	{
		oishii::BinaryReader reader(std::move(data), size, fileName.c_str());
		reader.seekSet(0);

		pk1::TXE txe;
		reader.dispatch<pk1::TXE, oishii::Direct, false>(txe);
	}

	fStream.close();
	return;
}
static void openDCAFile()
{
	auto selection = pfd::open_file("Select a file", ".", { "Pikmin 1 Animation Files (*.dca)", "*.dca" }, false).result();
	if (selection.empty()) // user has pressed cancel
		return;

	std::string fileName(selection[0]);
	DebugReport("Opening file %s\n", fileName.c_str());

	std::ifstream fStream(fileName, std::ios::binary | std::ios::ate);

	if (!fStream.is_open())
		return;

	u32 size = static_cast<u32>(fStream.tellg());
	fStream.seekg(0, std::ios::beg);

	auto data = std::unique_ptr<char>(new char[size]);
	if (fStream.read(data.get(), size))
	{
		oishii::BinaryReader reader(std::move(data), size, fileName.c_str());
		reader.seekSet(0);
		reader.setEndian(true);

		pk1::DCA dca;
		reader.dispatch<pk1::DCA, oishii::Direct, false>(dca);
	}

	fStream.close();
	return;
}
static void openDCKFile()
{
	auto selection = pfd::open_file("Select a file", ".", { "Pikmin 1 Animation Files (*.dck)", "*.dck" }, false).result();
	if (selection.empty()) // user has pressed cancel
		return;

	std::string fileName(selection[0]);
	DebugReport("Opening file %s\n", fileName.c_str());

	std::ifstream fStream(fileName, std::ios::binary | std::ios::ate);

	if (!fStream.is_open())
		return;

	u32 size = static_cast<u32>(fStream.tellg());
	fStream.seekg(0, std::ios::beg);

	auto data = std::unique_ptr<char>(new char[size]);
	if (fStream.read(data.get(), size))
	{
		oishii::BinaryReader reader(std::move(data), size, fileName.c_str());
		reader.seekSet(0);
		reader.setEndian(true);

		pk1::DCK dck;
		reader.dispatch<pk1::DCK, oishii::Direct, false>(dck);
	}

	fStream.close();
	return;
}

// J3D
static void openBTIFile()
{
	auto selection = pfd::open_file("Select a file", ".", { "J3D Texture Files (*.bti)", "*.bti" }, false).result();
	if (selection.empty()) // user has pressed cancel
		return;

	std::string fileName(selection[0]);
	DebugReport("Opening file %s\n", fileName.c_str());

	std::ifstream fStream(fileName, std::ios::binary | std::ios::ate);

	if (!fStream.is_open())
		return;

	u32 size = static_cast<u32>(fStream.tellg());
	fStream.seekg(0, std::ios::beg);

	auto data = std::unique_ptr<char>(new char[size]);
	if (fStream.read(data.get(), size))
	{
		oishii::BinaryReader reader(std::move(data), size, fileName.c_str());
		reader.seekSet(0);

		pk1::BTI bti;
		reader.dispatch<pk1::BTI, oishii::Direct, false>(bti);
	}

	fStream.close();
	return;
}

// Other

}
