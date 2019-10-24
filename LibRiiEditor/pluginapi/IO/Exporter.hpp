#pragma once

#include <oishii/writer/binary_writer.hxx>

namespace pl {

struct Exporter
{
	virtual ~Exporter() = default;

	virtual bool write(oishii::Writer& reader, FileState& state) = 0;
};

struct ExporterSpawner
{
	virtual ~ExporterSpawner() = default;

	virtual bool match(const std::string& id) = 0;
	virtual std::unique_ptr<Exporter> spawn() const = 0;
	virtual std::unique_ptr<ExporterSpawner> clone() const = 0;
};

} // namespace pl
