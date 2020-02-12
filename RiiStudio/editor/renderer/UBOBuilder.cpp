
#include <GL/gl3w.h>
#include "GLFW/glfw3.h"

#include "UBOBuilder.hpp"
#include <algorithm>

UBOBuilder::UBOBuilder()
{
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformStride);


    glGenBuffers(1, &UBO);
}

UBOBuilder::~UBOBuilder()
{
    glDeleteBuffers(1, &UBO);
}

void DelegatedUBOBuilder::submit()
{
    std::size_t size = 0;
    for (const auto& binding_point_collection : mData)
    {
        // The user must ensure these are aligned!
        for (const auto& binding_point_entry : binding_point_collection)
            size += binding_point_entry.size();
        size = roundUniformUp(size);
    }

    // if (size > mCoalesced.size())
        mCoalesced.resize(size);
	mCoalescedOffsets.clear();

    std::size_t cursor = 0;
    for (const auto& binding_point_collection : mData)
    {
        if (binding_point_collection.size() < 1)
        {
            mCoalescedOffsets.emplace_back(cursor, 0);
            continue;
        }

        mCoalescedOffsets.emplace_back(cursor, binding_point_collection[0].size());
        // The user must ensure these are aligned!
        for (const auto& binding_point_entry : binding_point_collection)
        {
			assert(cursor + binding_point_entry.size() <= size);
            memcpy(mCoalesced.data() + cursor, binding_point_entry.data(), binding_point_entry.size());
            cursor += binding_point_entry.size();
        }
        cursor = roundUniformUp(cursor);
    }

	assert(cursor == mCoalesced.size() && cursor == size);

    glBindBuffer(GL_UNIFORM_BUFFER, getUboId());
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, mCoalesced.data());

}

// Use the data at each binding point
void DelegatedUBOBuilder::use(u32 idx)
{

    for (int i = 0; i < mData.size(); ++i)
    {
        const auto& ofs = mCoalescedOffsets[i];
		assert(ofs.first + ofs.second * idx < mCoalesced.size());
        glBindBufferRange(GL_UNIFORM_BUFFER, i, getUboId(), ofs.first + ofs.second * idx, ofs.second * mData[i].size());
    }
}

void DelegatedUBOBuilder::push(u32 binding_point, const std::vector<u8>& data)
{
    if (binding_point >= mData.size())
        mData.resize(binding_point + 1);
	else
		assert(mData[binding_point].empty() || mData[binding_point][0].size() == data.size());
    mData[binding_point].push_back(data);
}
void DelegatedUBOBuilder::clear()
{
    mData.clear();
}


VBOBuilder::VBOBuilder()
{
	glGenBuffers(1, &mPositionBuf);
	glGenBuffers(1, &mIndexBuf);

	glGenVertexArrays(1, &VAO);
}
VBOBuilder::~VBOBuilder()
{
	glDeleteBuffers(1, &mPositionBuf);
	glDeleteBuffers(1, &mIndexBuf);

	glDeleteVertexArrays(1, &VAO);
}
void VBOBuilder::build()
{
	std::vector<std::pair<VAOEntry, u32>> mAttribStack; // desc : offset

	for (const auto& bind : mPropogating)
	{
		mAttribStack.emplace_back(bind.second.first, mData.size());

		for (const u8 e : bind.second.second)
			push(e);
	}

	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * 4, mIndices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mPositionBuf);
	glBufferData(GL_ARRAY_BUFFER, mData.size(), mData.data(), GL_STATIC_DRAW);
	glBindVertexArray(VAO);
	for (const auto& attrib : mAttribStack)
	{
		assert(attrib.first.format == GL_FLOAT);
		glVertexAttribPointer(attrib.first.binding_point, attrib.first.size/4, GL_FLOAT, GL_FALSE, attrib.first.size, (void*)(u32)attrib.second);
		//glVertexAttribPointer(attrib.first.binding_point, attrib.first.size, attrib.first.format, GL_FALSE, attrib.first.size, (void*)attrib.second);
		glEnableVertexAttribArray(attrib.first.binding_point);

	}

	mPropogating.clear();
}
