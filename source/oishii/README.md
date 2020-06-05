# Oishii

A powerful and performant C++ library for endian binary IO.
Pronounced oi-shi, from Japanese おいしい--"delicious".

## Reading files

Basic reader functionality has been provided, as well as several layers of compile-time abstraction.

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

More advanced abstraction is provided with dispatching. All done with template magic, no runtime cost is incurred.
A convenient READER_HANDLER macro has been provided to initialize a simple handler.

```cpp
using namespace oishii;

READER_HANDLER(Section2Handler, "Section 2 Data", DecodedSection2&)
{
	// Read data
}

void readFile(BinaryReader& reader, DecodedFile& file)
{
	// Read some members
	reader.dispatch<Section2Handler, Indirection<0, s32, Whence::Set>>(file.section2);
	// Continue to read other data
}
```
Dispatch calls are in the following form, and support chaining of indirection.
```cpp
template <typename THandler, 		// The handler. THandler::onRead will be called, passing the context.
	typename TIndirection = Direct, // The sequence necessary to derive the value.
	bool seekBack = true, 		// Whether or not the reader should be restored to the end of the first indirection jump.
	typename TContext> 		// Type of value to pass to handler.
void dispatch(TContext& ctx, /* ... */);
```

### Indirections

Indirections take the following form:
```cpp
template<int ITrans=0, 					// The translation of the pointer at this level. For example, specifying 4 would advance four bytes.
	typename TPointer = Indirection_None,   	// If this is an offset/pointer, the type of that pointer.
	Whence WPointer = Whence::Current, 		// Relation of the pointer offset.
	typename IndirectionNext = Indirection_None> 	// The next indirection. Chaining is supported.
	struct Indirection;
```

In Nintendo collision data, triangle data is one indexed, so the file header offset is set sizeof(Triangle) less.
We can intuitively express this with an indirection.

```cpp
reader.dispatch<BufferReader<Triangle>,
	Indirection<
		0, 			// Offset of pointer is not translated
		s32, 			// Offset is a signed 32 bit integer
		Whence::Set, 		// Seek from the start of file
		Indirection<-0x10> 	// Next indirection -- after jumped to pointer, go back 16 bytes.
	>>(mTriBuffer);
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

TODO: Add example linker map.

