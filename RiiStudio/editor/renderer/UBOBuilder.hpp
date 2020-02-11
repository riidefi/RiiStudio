#pragma once

#include <LibCore/common.h>
#include <vector>
#include <memory>
#include <tuple>

struct UBOBuilder
{
	UBOBuilder();
	~UBOBuilder();

	inline u32 roundUniformUp(u32 ofs) { return roundUp(ofs, uniformStride); }
	inline u32 roundUniformDown(u32 ofs) { return roundDown(ofs, uniformStride); }
	inline int getUniformAlignment() const { return uniformStride; }
    inline u32 getUboId() const { return UBO; }

private:
	int uniformStride = 0;
	u32 UBO;
};

// Prototype -- we can do this much more efficiently
struct DelegatedUBOBuilder : public UBOBuilder
{
    DelegatedUBOBuilder() = default;
    ~DelegatedUBOBuilder() = default;

MODULE_PRIVATE:
    void submit();

    // Use the data at each binding point
    void use(u32 idx);

MODULE_PUBLIC:
    virtual void push(u32 binding_point, const std::vector<u8>& data);

    template<typename T>
    inline void tpush(u32 binding_point, const T& data)
    {
        std::vector<u8> tmp(sizeof(T));
        *reinterpret_cast<T*>(tmp.data()) = data;
        push(binding_point, tmp);
    }

MODULE_PRIVATE:
    void clear();

private:
    // Indices as binding ids
    using RawData = std::vector<u8>;
    std::vector<std::vector<RawData>> mData;

    // Recomputed each submit
    std::vector<u8> mCoalesced;
    std::vector<std::pair<u32, u32>> mCoalescedOffsets; // Offset : Stride
};
