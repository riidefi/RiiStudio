#pragma once

#include <core/kpi/Node.hpp>
#include <tuple>
#include <string>

void InitAPI();
void DeinitAPI();

std::pair<std::string, std::unique_ptr<kpi::IBinaryDeserializer>> SpawnImporter(const std::string& fileName, oishii::BinaryReader& reader);
std::unique_ptr<kpi::IBinarySerializer> SpawnExporter(const std::string& type);
std::unique_ptr<kpi::IDocumentNode> SpawnState(const std::string& type);
// px::RichName GetRich(const std::string& type);

bool IsConstructible(const std::string& type);
