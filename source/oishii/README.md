# Oishii

A powerful and performant C++ library for endian binary IO.
Pronounced oi-shī, from Japanese おいしい--"delicious".

## Reading files

Basic reader functionality has been provided, as well as several layers of compile-time abstraction.

```cpp
// Reads two big-endian ulongs (representing minutes and seconds)
Result<f64> ReadMinutes(std::string_view path) {
  auto reader = TRY(oishii::BinaryReader::FromFilePath(path, std::endian::big));
  u32 minutes = TRY(reader.tryRead<u32>());
  u32 seconds = TRY(reader.tryRead<u32>());
  return static_cast<f64>(minutes) + static_cast<f64>(seconds) / 60.0;
}
```

```cpp
// Writes as two big-endian ulongs
void WriteMinutes(std::string_view path, f64 minutes) {
	oishii::Writer writer(std::endian::big);
	writer.write<u32>(static_cast<u32>(round(minutes, 60.0));
	writer.write<u32>(static_cast<u32>(fmod(minutes, 60.0) * 60.0);
	writer.saveToDisk(path);
}
```

Reader/writer breakpoints allow you to quickly inspect which function is reading/writing data a certain offset. This can be very useful for determining what structure/field corresponds to an arbitrary file location, which may be quite difficult with complex formats.
```cpp
// Reader breakpoint
void ReadBP(std::string_view path) {
	auto reader = oishii::BinaryReader::FromFilePath(path, std::endian::big).value();
	reader.add_bp<u32>(0x10);
	
	// Trigger BP
	reader.seekSet(0x10);
	u32 _ = reader.tryRead<u32>(0).value();
}
```

For instance, the output may look like this:
```
test.bin:0x10: warning: Breakpoint hit
        Offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
        000010  53 45 43 31 CD CD CD CD CD CD CD CD CD CD CD CD SEC1............
                ^~~~~~~~~~~                                     ^~~~
```

For debugging file rebuilding, a ground-truth copy can be provided. If the writer writes data that does not match the ground-truth copy, it will immediately cause the debugger to pause, at the function with the problem. In unit tests, it will cause an unhandled exception and print a full stacktrace.
```cpp
// Writer test case
void Test() {
	oishii::Writer writer(std::endian::big);

	std::vector<u8, 4> expected{0x12, 0x34, 0x56, 0x78};
	writer.attachDataForMatchingOutput(expected);

	// Error if does not match
	writer.write<u32>(0x1234'5678);
}
```

Constructing a reader from memory directly is also supported. In practice, this is the most common method of construction.
```cpp
struct PacketHeader {
	uint32_t signature;
	uint32_t len;
};
Result<PacketHeader> ReadPacketHeader(std::span<const u8> bytes) {
	// The |path| field is used for debug error messages
	oishii::BinaryReader reader(bytes, "Network buffer", std::endian::big);
	return PacketHeader {
		.signature = TRY(reader.tryRead<uint32_t>()),
		.len = TRY(reader.tryRead<uint32_t>()),
	};
}
```

Debug frames are supported for more meaningful warnings.
```cpp
Result<f64> ScopeTest(std::string_view path) {
  auto reader = TRY(oishii::BinaryReader::FromFilePath(path, std::endian::big));
  {
    auto scope = reader.createScoped("BMD root");
    reader.skip(0x20);
    {
      auto scope = reader.createScoped("INF1 section");
      reader.warnAt("Example message", reader.tell(), reader.tell() + 4);
    }
  }
}
```

Scope-based jumping forms the most basic level. No runtime cost is incurred.

```cpp
using namespace oishii;

void readFile(BinaryReader& reader)
{
	// Read some members
	{
		Jump<Whence::Current> scoped(reader, reader.read<u32>());

		// At end of scope, reader will automatically seek back
	}
	// Continue to read other data
}
```

This is equivalent to:

```cpp
using namespace oishii;

void readFile(BinaryReader& reader)
{
	// Read some members
	u32 section2_offset = reader.read<u32>();
	const auto back = reader.tell();

	reader.seek<Whence::Set>(section2_offset);
	// Read some members

	reader.seek<Whence::Set>(back);
	// Continue to read other data
}
```

### Invalidity tracking

By default, a light stack is updated on dispatch entry/exit, tracking the current reader position.
When the reader signals an invalidity, a stack trace is printed. This has been designed to be incredibly easy to understand and debug.
For example, let's rewrite our reader above to expect a section identifier, `SEC2`:
```cpp
READER_HANDLER(Section2Handler, "Section 2 Data", DecodedSection2&)
{
	reader.expectMagic<'SEC2'>();
	// Read data
}
```
If this expectation fails, the following will be printed.
```
test.bin:0x10: warning: File identification magic mismatch: expecting SEC2, instead saw SEC1.
        Offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
        000010  53 45 43 31 CD CD CD CD CD CD CD CD CD CD CD CD SEC1............
                ^~~~~~~~~~~                                     ^~~~
                In Section 2 Data: start=0x10, at=0x10
                In <root>: start=0x0, at=0x0
                        Offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
                        000000  00 00 00 10 CD CD CD CD CD CD CD CD CD CD CD CD ................
                                ^~~~~~~~~~~                                     ^~~~
```
Note: On Windows, error output will be colored (warnings will be yellow, errors red, and underlines green).

Note: expectMagic<magic> is a wrapper for this:
```cpp
if (read<u32, EndianSelect::Big>() != magic)
	signalInvalidityLast<u32, MagicInvalidity<magic>>();
```
Invalidities adhere to a simple interface and can be easily extended for application-specific needs.

TODO: document other reader features.


## Writing files
All of the basic IO features of reading are supported for writing. Scope based jumping may be used for writing as well.

For more sophisticated writing, a model based on how native applications are built. While writing a file, the user will label data and insert references. Then, the user will instruct the reader to link the file, resolving all references.
This labeling and referencing is done through a hierarchy of nodes (data blocks). Nodes will declare linking restrictions, such as alignment, and when called, will supply child nodes. The linker will recurse through the nodes, filling a layout. Once this layout is filled, the linker may reorder data to be more space efficient or respect linking restrictions. Then, the linker will iterate through this layout, calling the relevant serialization events on the specified nodes, accumulating a list of references to resolve. Once done, the linker will resolve all references and return control to the user.

References may either be in-memory pointers, or namespaced string identifiers. Each node exists in the namespace of its parent, and follows namespacing rules based on C++.
1) A node may reference one of its siblings or children without supplying its own namespace / the namespace of its parent.
2) A node may reference nodes beyond its immediate surroundings by providing a relative or absolute path.

The benefit of supporting this as well as node pointers for identifiers is that data that does not exist yet may be referenced. For example, the actual data block nodes for the rest of a file may not exist at the time the header is being serialized.

Back to our collision example, writing this could be expressed like so:
```cpp
// Data offset 1 indexed, store pointer 0x10 early
writer.writeLink<s32>(Link{ // The linker will resolve this to the distance between hooks
	Hook(*this), // Hook the position of this node in the file
	Hook("TriangleBuffer", DataBlock::Hook::Begin, -0x10) // Hook the beginning of the TriangleBuffer, minus our translation.
	});
```

### Linker Maps

The linker can optionally output a linker map, documenting node positions and ends, as well as hierarchy and linker restrictions. This can be quite useful for debugging the file itself.
This data can also be used to visualize space usage in a file.

Example linker map:
```
Begin    End      Size     Align    Static Leaf  Symbol
0x000000 0x000020 0x000020 0x000000 false  false
0x000020 0x000038 0x000018 0x000020 false  false INF1 InFormation
0x000038 0x00023c 0x000204 0x000000 false  true  INF1 InFormation::SceneGraph
0x00023c 0x00023c 0x000000 0x000000 false  true  INF1 InFormation::EndOfChildren
0x000240 0x000280 0x000040 0x000020 false  false VTX1
0x000280 0x0002d0 0x000050 0x000000 false  false VTX1::Format
0x0002d0 0x0002d0 0x000000 0x000000 false  true  VTX1::Format::EndOfChildren
0x0002e0 0x003f82 0x003ca2 0x000020 false  true  VTX1::Buf0
0x003fa0 0x007fd8 0x004038 0x000020 false  true  VTX1::Buf1
0x007fe0 0x008064 0x000084 0x000020 false  true  VTX1::Buf2
0x008080 0x009ee4 0x001e64 0x000020 false  true  VTX1::Buf3
0x009ee4 0x009ee4 0x000000 0x000000 false  true  VTX1::EndOfChildren
0x009f00 0x009f1c 0x00001c 0x000020 false  false EVP1
0x009f1c 0x009f29 0x00000d 0x000000 false  true  EVP1::MatrixSizeTable
0x009f29 0x009f5f 0x000036 0x000000 false  true  EVP1::MatrixIndexTable
0x009f60 0x009fcc 0x00006c 0x000008 false  true  EVP1::MatrixWeightTable
0x009fcc 0x00a56c 0x0005a0 0x000000 false  true  EVP1::MatrixInvBindTable
0x00a56c 0x00a56c 0x000000 0x000000 false  true  EVP1::EndOfChildren
0x00a580 0x00a594 0x000014 0x000020 false  false DRW1
0x00a594 0x00a5c4 0x000030 0x000000 false  false DRW1::MatrixTypeTable
0x00a5c4 0x00a5c4 0x000000 0x000000 false  true  DRW1::MatrixTypeTable::EndOfChildren
0x00a5c4 0x00a624 0x000060 0x000002 false  false DRW1::DataTable
0x00a624 0x00a624 0x000000 0x000000 false  true  DRW1::DataTable::EndOfChildren
0x00a624 0x00a624 0x000000 0x000000 false  true  DRW1::EndOfChildren
0x00a640 0x00a658 0x000018 0x000020 false  false JNT1
0x00a658 0x00add8 0x000780 0x000004 false  true  JNT1::JointData
0x00add8 0x00ae14 0x00003c 0x000002 false  true  JNT1::JointLUT
0x00ae14 0x00af6d 0x000159 0x000004 false  true  JNT1::JointNames
0x00af6d 0x00af6d 0x000000 0x000000 false  true  JNT1::EndOfChildren
0x00af80 0x00afac 0x00002c 0x000020 false  false SHP1
0x00afac 0x00b114 0x000168 0x000004 false  true  SHP1::ShapeData
0x00b114 0x00b126 0x000012 0x000004 false  true  SHP1::LUT
0x00b140 0x00b140 0x000000 0x000020 false  false SHP1::VCDList
0x00b140 0x00b168 0x000028 0x000010 false  true  SHP1::VCDList::0
0x00b170 0x00b1a0 0x000030 0x000010 false  true  SHP1::VCDList::1
0x00b1a0 0x00b1a0 0x000000 0x000000 false  true  SHP1::VCDList::EndOfChildren
0x00b1a0 0x00b1a0 0x000000 0x000000 false  false SHP1::MTXList
0x00b1a0 0x00b1a2 0x000002 0x000002 false  true  SHP1::MTXList::0
0x00b1a2 0x00b1a4 0x000002 0x000002 false  true  SHP1::MTXList::1
0x00b1a4 0x00b1a6 0x000002 0x000002 false  true  SHP1::MTXList::2
0x00b1a6 0x00b1a8 0x000002 0x000002 false  true  SHP1::MTXList::3
0x00b1a8 0x00b1aa 0x000002 0x000002 false  true  SHP1::MTXList::4
0x00b1aa 0x00b1ac 0x000002 0x000002 false  true  SHP1::MTXList::5
0x00b1ac 0x00b1ae 0x000002 0x000002 false  true  SHP1::MTXList::6
0x00b1ae 0x00b1b0 0x000002 0x000002 false  true  SHP1::MTXList::7
0x00b1b0 0x00b1c4 0x000014 0x000002 false  true  SHP1::MTXList::8
0x00b1c4 0x00b1d8 0x000014 0x000002 false  true  SHP1::MTXList::9
0x00b1d8 0x00b1ec 0x000014 0x000002 false  true  SHP1::MTXList::10
0x00b1ec 0x00b200 0x000014 0x000002 false  true  SHP1::MTXList::11
0x00b200 0x00b200 0x000000 0x000000 false  true  SHP1::MTXList::EndOfChildren
0x00b200 0x00b200 0x000000 0x000020 false  false SHP1::DLData
0x00b200 0x00b200 0x000000 0x000020 false  false SHP1::DLData::0
0x00b200 0x00c580 0x001380 0x000020 false  true  SHP1::DLData::0::0
0x00c580 0x00c580 0x000000 0x000000 false  true  SHP1::DLData::0::EndOfChildren
0x00c580 0x00c580 0x000000 0x000020 false  false SHP1::DLData::1
0x00c580 0x00dd80 0x001800 0x000020 false  true  SHP1::DLData::1::0
0x00dd80 0x00dd80 0x000000 0x000000 false  true  SHP1::DLData::1::EndOfChildren
0x00dd80 0x00dd80 0x000000 0x000020 false  false SHP1::DLData::2
0x00dd80 0x00e280 0x000500 0x000020 false  true  SHP1::DLData::2::0
0x00e280 0x00e280 0x000000 0x000000 false  true  SHP1::DLData::2::EndOfChildren
0x00e280 0x00e280 0x000000 0x000020 false  false SHP1::DLData::3
0x00e280 0x00e3c0 0x000140 0x000020 false  true  SHP1::DLData::3::0
0x00e3c0 0x00e3c0 0x000000 0x000000 false  true  SHP1::DLData::3::EndOfChildren
0x00e3c0 0x00e3c0 0x000000 0x000020 false  false SHP1::DLData::4
0x00e3c0 0x00e540 0x000180 0x000020 false  true  SHP1::DLData::4::0
0x00e540 0x00e540 0x000000 0x000000 false  true  SHP1::DLData::4::EndOfChildren
0x00e540 0x00e540 0x000000 0x000020 false  false SHP1::DLData::5
0x00e540 0x011580 0x003040 0x000020 false  true  SHP1::DLData::5::0
0x011580 0x011580 0x000000 0x000000 false  true  SHP1::DLData::5::EndOfChildren
0x011580 0x011580 0x000000 0x000020 false  false SHP1::DLData::6
0x011580 0x0125c0 0x001040 0x000020 false  true  SHP1::DLData::6::0
0x0125c0 0x0125c0 0x000000 0x000000 false  true  SHP1::DLData::6::EndOfChildren
0x0125c0 0x0125c0 0x000000 0x000020 false  false SHP1::DLData::7
0x0125c0 0x013580 0x000fc0 0x000020 false  true  SHP1::DLData::7::0
0x013580 0x013580 0x000000 0x000000 false  true  SHP1::DLData::7::EndOfChildren
0x013580 0x013580 0x000000 0x000020 false  false SHP1::DLData::8
0x013580 0x0143a0 0x000e20 0x000020 false  true  SHP1::DLData::8::0
0x0143a0 0x016c60 0x0028c0 0x000020 false  true  SHP1::DLData::8::1
0x016c60 0x017940 0x000ce0 0x000020 false  true  SHP1::DLData::8::2
0x017940 0x01a100 0x0027c0 0x000020 false  true  SHP1::DLData::8::3
0x01a100 0x01a100 0x000000 0x000000 false  true  SHP1::DLData::8::EndOfChildren
0x01a100 0x01a100 0x000000 0x000000 false  true  SHP1::DLData::EndOfChildren
0x01a100 0x01a100 0x000000 0x000004 false  false SHP1::MTXData
0x01a100 0x01a108 0x000008 0x000004 false  true  SHP1::MTXData::0
0x01a108 0x01a110 0x000008 0x000004 false  true  SHP1::MTXData::1
0x01a110 0x01a118 0x000008 0x000004 false  true  SHP1::MTXData::2
0x01a118 0x01a120 0x000008 0x000004 false  true  SHP1::MTXData::3
0x01a120 0x01a128 0x000008 0x000004 false  true  SHP1::MTXData::4
0x01a128 0x01a130 0x000008 0x000004 false  true  SHP1::MTXData::5
0x01a130 0x01a138 0x000008 0x000004 false  true  SHP1::MTXData::6
0x01a138 0x01a140 0x000008 0x000004 false  true  SHP1::MTXData::7
0x01a140 0x01a160 0x000020 0x000004 false  true  SHP1::MTXData::8
0x01a160 0x01a160 0x000000 0x000000 false  true  SHP1::MTXData::EndOfChildren
0x01a160 0x01a160 0x000000 0x000002 false  false SHP1::MTXGrpHdr
0x01a160 0x01a168 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::0
0x01a168 0x01a170 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::1
0x01a170 0x01a178 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::2
0x01a178 0x01a180 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::3
0x01a180 0x01a188 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::4
0x01a188 0x01a190 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::5
0x01a190 0x01a198 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::6
0x01a198 0x01a1a0 0x000008 0x000004 false  true  SHP1::MTXGrpHdr::7
0x01a1a0 0x01a1c0 0x000020 0x000004 false  true  SHP1::MTXGrpHdr::8
0x01a1c0 0x01a1c0 0x000000 0x000000 false  true  SHP1::MTXGrpHdr::EndOfChildren
0x01a1c0 0x01a1c0 0x000000 0x000000 false  true  SHP1::EndOfChildren
0x01a1c0 0x01bd30 0x001b70 0x000020 false  false MAT3
0x01bd30 0x01bd30 0x000000 0x000000 false  true  MAT3::EndOfChildren
0x01bd40 0x01bd54 0x000014 0x000020 false  false TEX1
0x01bd60 0x01bd60 0x000000 0x000020 false  false TEX1::TexHeaders
0x01bd60 0x01bd80 0x000020 0x000004 false  true  TEX1::TexHeaders::0
0x01bd80 0x01bda0 0x000020 0x000004 false  true  TEX1::TexHeaders::1
0x01bda0 0x01bdc0 0x000020 0x000004 false  true  TEX1::TexHeaders::2
0x01bdc0 0x01bde0 0x000020 0x000004 false  true  TEX1::TexHeaders::3
0x01bde0 0x01be00 0x000020 0x000004 false  true  TEX1::TexHeaders::4
0x01be00 0x01be20 0x000020 0x000004 false  true  TEX1::TexHeaders::5
0x01be20 0x01be40 0x000020 0x000004 false  true  TEX1::TexHeaders::6
0x01be40 0x01be60 0x000020 0x000004 false  true  TEX1::TexHeaders::7
0x01be60 0x01be80 0x000020 0x000004 false  true  TEX1::TexHeaders::8
0x01be80 0x01bea0 0x000020 0x000004 false  true  TEX1::TexHeaders::9
0x01bea0 0x01bec0 0x000020 0x000004 false  true  TEX1::TexHeaders::10
0x01bec0 0x01bee0 0x000020 0x000004 false  true  TEX1::TexHeaders::11
0x01bee0 0x01bf00 0x000020 0x000004 false  true  TEX1::TexHeaders::12
0x01bf00 0x01bf20 0x000020 0x000004 false  true  TEX1::TexHeaders::13
0x01bf20 0x01bf40 0x000020 0x000004 false  true  TEX1::TexHeaders::14
0x01bf40 0x01bf40 0x000000 0x000000 false  true  TEX1::TexHeaders::EndOfChildren
0x01bf40 0x01ff40 0x004000 0x000020 false  true  TEX1::0
0x01ff40 0x023f40 0x004000 0x000020 false  true  TEX1::1
0x023f40 0x027f40 0x004000 0x000020 false  true  TEX1::2
0x027f40 0x028f40 0x001000 0x000020 false  true  TEX1::3
0x028f40 0x029f40 0x001000 0x000020 false  true  TEX1::4
0x029f40 0x02af40 0x001000 0x000020 false  true  TEX1::5
0x02af40 0x02cf40 0x002000 0x000020 false  true  TEX1::6
0x02cf40 0x02ef40 0x002000 0x000020 false  true  TEX1::7
0x02ef40 0x02f740 0x000800 0x000020 false  true  TEX1::8
0x02f740 0x030740 0x001000 0x000020 false  true  TEX1::9
0x030740 0x030f40 0x000800 0x000020 false  true  TEX1::10
0x030f40 0x031740 0x000800 0x000020 false  true  TEX1::11
0x031740 0x031f40 0x000800 0x000020 false  true  TEX1::12
0x031f40 0x032740 0x000800 0x000020 false  true  TEX1::13
0x032740 0x034740 0x002000 0x000020 false  true  TEX1::14
0x034740 0x034840 0x000100 0x000004 false  true  TEX1::TexNames
0x034840 0x034840 0x000000 0x000000 false  true  TEX1::EndOfChildren
0x034840 0x034840 0x000000 0x000000 false  true  EndOfChildren
```
