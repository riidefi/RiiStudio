/*! @file

Typically, CLI tools are used in terms of actions.
>> gcc -c file.c -o file.o
In this setup, the tool maintains a temporary state; actions are the transition from "c file" to "object file" or "elf file" to "slightly different elf file".
We can think of it like this:
	ELF -> (Internal Representation) -> DOL
		^~							 ^~
RS_CLI takes a slightly different approach. Rather than describing work in terms of connections, work is described in terms of endpoints.
	ELF -> (Internal Representation) -> DOL
	^~	   (^~)							^~

For example, say RS_CLI is loaded with plugins: one for Pikmin 1 MOD models and one for JSystem BMD models that both implement one common interface,
say "GameCube Model". Through this link, RS_CLI can specify any number of conversions assuming both endpoints have a valid route.

However, problems may arise if ambiguities are present. Consider the following example output:
>> Cannot proceed -- Conversion between BMD and MDL has ambigious conversion stream: specify `GameCubeGPUStream` or `GameCubeAPIStream`
>> - GameCubeGPUStream: Conversion via gpu command generation and api reconstruction -- CPU effects lost
>> - GameCubeAPIStream: Conversion via api abstraction transformation
It thus may be necessary to explicitly provide intermediate steps to guide conversion.

That's great, but often we need far more than just format conversion. How do we express traditional actions?
Internally, we use the same interface that generates GUI toolbar actions.
A "transform stack", often abbreviated "xform stack", represents a stack of alterations that may be applied to a state before moving on to the next endpoint.
>> RS_CLI MOD source.mod -recompute_normals=broken MOD source.mod
We also introduce two shortcuts to make this more fluid:
	1) The initial format can be inferred in most cases.
	2) Both the terminal format and file, if left empty, are implied to be their initial counterpart.
>> RS_CLI source.mod -recompute_normals=broken
While in this instance, functionality is the same as traditional CLIs, we can very easily add a step:
>> RS_CLI source.mod -recompute_normals=broken BMD
Or more:
>> RS_CLI source.mod -recompute_normals=broken BMD [out.mod] -uv_quantize=fixed14

By assuming the responsibility of state management from the user, we already perform far better given:
	1) We don't need to write to the notriously slow disc.
	2) We don't need to serialize and deserialize data. We can immediately use our internal representation.
		->	While orchestrating the many endpoints and connections in our system, we can often elide
			internal states entirely by directly shuttling information through our interface network.


Grammar: <tool> [[format] [path] [options]]*
Note: While syntatically optional, the first path is semantically necessary.
Note: The ambiguity between [format] and [path] is currently solved in giving unquestioned precedence to [format].
*/

#pragma once

#include <LibRiiEditor/core/PluginFactory.hpp>
#include <oishii/reader/binary_reader.hxx>
#include "session.hpp"

namespace rs_cli {

class System
{
public:
	//! The constructor: Installs LibCube, begins the session.
	//!
	System(int argc, char* const* argv);
	~System();

	struct SpawnedImporter
	{
		std::string fileStateId;
		std::unique_ptr<pl::Importer> importer;
		// Added
		std::unique_ptr<oishii::BinaryReader> mReader;

	};
	std::optional<SpawnedImporter> spawnImporter(const std::string& path);
	std::unique_ptr<pl::Exporter> spawnExporter(const std::string& id);
	std::unique_ptr<pl::FileState> spawnFileState(const std::string& id);

private:
	Session mSession;
	PluginFactory mPluginFactory;
};


} // namespace rs_cli
