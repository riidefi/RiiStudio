#pragma once

#include <core/common.h>
#include <string>
#include <vector>

namespace oishii {
	class BinaryReader;
	namespace v2 { struct Writer; }
}

namespace riistudio::g3d {

struct DictionaryNode {
    u16 mId = 0;
    u16 mFlag = 0;
    u16 mIdxPrev = 0;
    u16 mIdxNext = 0;

    void resetInternal() { mId = mFlag = mIdxPrev = mIdxNext = 0; }
    std::string mName;

    s32 mDataDestination = -1;

    inline bool calcId(u16 id) {
        const u32 c = id >> 3;
        
        return c < mName.size() && ((mName[c] >> (id & 7)) & 1);
    }

    void read(oishii::BinaryReader& reader, u32 start);
    // void write(oishii::v2::Writer& writer, u32 start) const;

    DictionaryNode() = default;
    inline DictionaryNode(oishii::BinaryReader& reader, u32 start) { read(reader, start); }
};

struct Dictionary {
    std::vector<DictionaryNode> mNodes;

    void calcNode(u32 id);
    void calcNodes();

    void read(oishii::BinaryReader& reader);
    void write(oishii::v2::Writer& writer);

	Dictionary() = default;
	Dictionary(oishii::BinaryReader& reader) {
		read(reader);
	}
};



}
