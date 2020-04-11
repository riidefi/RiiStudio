#pragma once

#include <core/common.h>
#include <array>

namespace libcube {


template<typename Feature>
struct TPropertySupport
{
	enum class Coverage
	{
		Unsupported,
		Read,
		ReadWrite
	};


	Coverage supports(Feature f) const noexcept
	{
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		if (static_cast<u64>(f) >= static_cast<u64>(Feature::Max))
			return Coverage::Unsupported;

		return registration[static_cast<u64>(f)];
	}
	bool canRead(Feature f) const noexcept
	{
		return supports(f) == Coverage::Read || supports(f) == Coverage::ReadWrite;
	}
	bool canWrite(Feature f) const noexcept
	{
		return supports(f) == Coverage::ReadWrite;
	}
	void setSupport(Feature f, Coverage s)
	{
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		if (static_cast<u64>(f) < static_cast<u64>(Feature::Max))
			registration[static_cast<u64>(f)] = s;
	}

	Coverage& operator[](Feature f) noexcept
	{
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		return registration[static_cast<u64>(f)];
	}
	Coverage operator[](Feature f) const noexcept
	{
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		return registration[static_cast<u64>(f)];
	}
private:
	std::array<Coverage, static_cast<u64>(Feature::Max)> registration;
};

/*
template<typename T>
struct Property
{
	virtual ~Property() = default;
	//	bool writeable = true;

	virtual T get() = 0;
	virtual void set(T) = 0;
};

template<typename T>
struct RefProperty : public Property<T>
{
	RefProperty(T& r) : mRef(r) {}
	~RefProperty() override = default;

	T get() override { return mRef; }
	void set(T v) override { mRef = v; };

	T& mRef;
};
*/

}
