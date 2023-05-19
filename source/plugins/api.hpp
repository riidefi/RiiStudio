#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/Plugins.hpp>
#include <string>
#include <tuple>

void InitAPI();
void DeinitAPI();

std::pair<std::string, std::unique_ptr<kpi::IBinaryDeserializer>>
SpawnImporter(const std::string& fileName, std::span<const u8> reader);
std::unique_ptr<kpi::IBinarySerializer> SpawnExporter(kpi::INode& node);
std::unique_ptr<kpi::IObject> SpawnState(const std::string& type);
// px::RichName GetRich(const std::string& type);

bool IsConstructible(const std::string& type);
std::vector<std::string> GetChildrenOfType(const std::string& type);
