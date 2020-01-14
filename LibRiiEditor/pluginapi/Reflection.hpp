#pragma once

#include <string_view>

namespace pl {

// Not the most clean reflection implementation but it meets all of the minimum requirements.
//
struct MirrorEntry
{
	const std::string_view derived;
	const std::string_view base;
	int translation;

	MirrorEntry(const std::string_view d, const std::string_view b, int t)
		: derived(d), base(b), translation(t)
	{}
};


template<typename D, typename B>
constexpr int computeTranslation()
{
	return reinterpret_cast<char*>(static_cast<B*>(reinterpret_cast<D*>(0x10000000))) - reinterpret_cast<char*>(0x10000000);
}

} // namespace pl
