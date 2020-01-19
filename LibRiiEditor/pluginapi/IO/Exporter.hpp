#pragma once

#include <oishii/v2/writer/binary_writer.hxx>

namespace pl {

struct Exporter
{
	virtual ~Exporter() = default;

	virtual bool write(oishii::v2::Writer& reader, FileState& state) = 0;
};

struct ExporterSpawner
{
	virtual ~ExporterSpawner() = default;

	virtual bool match(const std::string& id) = 0;
	virtual std::unique_ptr<Exporter> spawn() const = 0;
	virtual std::unique_ptr<ExporterSpawner> clone() const = 0;
};

} // namespace pl
