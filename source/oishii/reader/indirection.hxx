#pragma once

#include "../interfaces.hxx"
#include <type_traits>

namespace oishii {

struct Indirection_None
{
	static constexpr bool is_pointed = false;
	typedef int offset_t;
	operator int() { return 0; }


	static constexpr Whence whence = Whence::Current;

	static constexpr int translation = 0;

	static constexpr bool redirects = false;
	static constexpr bool has_next = false;


	typedef Indirection_None next;
};

//! @tparam	translation	The offset of the pointer.
//! @tparam TPointer	The type of the offset.
//! @tparam WPointer	The relation of the pointer offset.
template<int ITrans,
	typename TPointer = Indirection_None, Whence WPointer = Whence::Current,
	typename IndirectionNext = Indirection_None>
	struct Indirection
{
	// traits
	struct indirection_t {};

	//
	typedef TPointer offset_t;
	static constexpr Whence whence = WPointer;
	static constexpr int translation = ITrans;

	typedef IndirectionNext next;

	//! If the indirection actually redirects the flow.
	//!
	static constexpr bool redirects = ITrans || WPointer != Whence::Current || !std::is_same<TPointer, Indirection_None>::value;

	//! If the next link exists.
	//!
	static constexpr bool has_next = !std::is_same<IndirectionNext, Indirection_None>::value;// && IndirectionNext::redirects;

	static constexpr bool is_pointed = !std::is_same<TPointer, Indirection_None>::value;
};

using Direct = Indirection<0, Indirection_None, Whence::Current, Indirection_None>;

static_assert(!Direct::redirects, "Indirection traits failed to detect nonindirection.");
static_assert(Indirection<4, int, Whence::Current, int>::redirects &&
	Indirection<0, int, Whence::Set, int>::redirects,
	"Indirection traits failed to detect indirection.");

} // namespace oishii