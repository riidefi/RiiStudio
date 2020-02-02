#pragma once

#include <LibCore/api/Node.hpp>
#include <tuple>
#include <string>

void InitAPI();
void DeinitAPI();

std::pair<std::string, std::unique_ptr<px::IBinarySerializer>> SpawnImporter(const std::string& fileName, oishii::BinaryReader& reader);
std::unique_ptr<px::IBinarySerializer> SpawnExporter(const std::string& type);
px::Dynamic SpawnState(const std::string& type);
px::RichName GetRich(const std::string& type);
