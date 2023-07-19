#pragma once

// Utilities to read directly from Mario Kart Wii's memory.
//
// The API is io-agnostic, and should be compatible with RAM dumps, live memory
// or some over-serial debugger.

#include <assert.h>
#include <functional>
#include <optional>
#include <rsl/SimpleReader.hpp>

namespace librii::live_mkw {

// `fread`, essentially
using Io = std::function<bool(u32, std::span<u8>)>;

template <typename T> Result<T> ReadFromDolphin(Io io, u32 addr) {
  T raw;
  bool ok = io(addr, {(u8*)&raw, sizeof(raw)});
  if (!ok) {
    return std::unexpected(
        std::format("Failed to read {} bytes from {:x}", sizeof(raw), addr));
  }
  return raw;
}

enum class ResourceChannelID {
  RES_CHAN_RACE_SYS, //!< [0] Localized: /Race/Common.szs and such
  RES_CHAN_COURSE,   //!< [1] Diff'd: Track file
  RES_CHAN_UI,       //!< [2] Localized: 2D UI
  RES_CHAN_FONT,     //!< [3] Fonts
  RES_CHAN_EARTH,    //!< [4] Globe (Async LZ)
  RES_CHAN_MII,      //!< [5] Mii Parts
  RES_CHAN_DRIVER,   //!< [6] Driver
  RES_CHAN_DEMO,     //!< [7] Cutscenes
  RES_CHAN_UI_MODEL, //!< [8] BackModel et al

  RES_CHAN_PLAYER = 0xA,
};
enum class EResourceKind {
  RES_KIND_FILE_DOUBLE_FORMAT, // 0 %s%s Supports prefix
  RES_KIND_FILE_SINGLE_FORMAT, // 1 %s
  RES_KIND_BUFFER,             // 2
  RES_KIND_3,                  // unseen
  RES_KIND_4,                  // _Dif loader

  RES_KIND_DEFAULT = RES_KIND_FILE_DOUBLE_FORMAT
};

enum class ArchiveState {
  DVD_ARCHIVE_STATE_CLEARED = 0,
  DVD_ARCHIVE_STATE_RIPPED = 2,
  DVD_ARCHIVE_STATE_DECOMPRESSED = 3,
  DVD_ARCHIVE_STATE_MOUNTED = 4,
  DVD_ARCHIVE_STATE_UNKN5 = 5
};
template <typename T> struct ptr {
  Result<T> get(Io io, u32 idx = 0) const {
    return ReadFromDolphin<T>(io, data + idx * sizeof(T));
  }
  rsl::bu32 data;
};
struct DvdArchive {
  rsl::bu32 vtable;
  ptr<void> mArchive;
  ptr<void> mArchiveStart;
  rsl::bu32 mArchiveSize;
  ptr<void> mArchiveHeap;
  ptr<void> mFileStart;
  rsl::bu32 mFileSize;
  ptr<void> mFileHeap;
  rsl::bu32 mStatus;
};
static_assert(sizeof(DvdArchive) == 0x24);
struct MultiDvdArchive {
  rsl::bu32 vtable;
  ptr<DvdArchive> archives;
  rsl::bu16 archiveCount;
  u16 _;
  ptr<rsl::bu32> fileSizes;
  ptr<ptr<char>> suffixes;
  ptr<ptr<void>> fileStarts;
  ptr<rsl::bu32> kinds;
};
static_assert(sizeof(MultiDvdArchive) == 0x1c);

//! Bidirectional list node
struct ut_Node {
  ptr<void> pred;
  ptr<void> succ;
};
static_assert(sizeof(ut_Node) == 0x8);

// Unlike modern "std::list"-like structures, list nodes are directly inherited
// by children, which saves a level of indirection.
struct ut_List {
  ptr<void> head;
  ptr<void> tail;
  rsl::bu16 count;
  rsl::bu16 intrusion_offset;
};
static_assert(sizeof(ut_List) == 0xC);

static inline Result<std::vector<u32>> ReadUtList(Io io, const ut_List& list) {
  std::vector<u32> result;
  u32 it = list.head.data;
  for (u32 i = 0; i < list.count; ++i) {
    EXPECT(it != 0 && "Corrupted linked list");
    u32 inner = it + list.intrusion_offset;
    auto node = TRY(ReadFromDolphin<ut_Node>(io, inner));

    result.push_back(it);

    it = node.succ.data;
  }
  return result;
}

struct GameScene {
  u8 _00[0xC74 - 0x000];
  ut_List mArchiveList;
};
struct GameArcInfo {
  ut_Node node;
  ptr<MultiDvdArchive> arc;
  rsl::bu32 type;
};
static_assert(sizeof(GameArcInfo) == 16);

struct Info {
  ResourceChannelID type;
  MultiDvdArchive arc;
};
static inline Result<std::vector<Info>>
GameScene_ReadArchives(Io io, const GameScene& scn) {
  auto arcs = TRY(ReadUtList(io, scn.mArchiveList));
  std::vector<Info> result;
  for (u32 u : arcs) {
    auto inf = TRY(ReadFromDolphin<GameArcInfo>(io, u));
    Info my_info;
    my_info.type = (ResourceChannelID)(u32)inf.type;
    my_info.arc = TRY(inf.arc.get(io));
    result.push_back(my_info);
  }
  return result;
}
static inline Result<u32> ReadChain(Io io, std::span<u32> ofs) {
  u32 it = 0;
  for (auto x : ofs) {
    it = TRY(ReadFromDolphin<rsl::bu32>(io, it + x));
    if (it == 0) {
      return 0;
    }
  }
  return it;
}
static inline Result<GameScene> GetGameScene(Io io) {
  u32 scene_id_chain[] = {0x80385FC8, 0x54, 0xC, 0x28};
  u32 scene_id = TRY(ReadChain(io, scene_id_chain));
  if (scene_id == 0 || scene_id == 5) {
    return std::unexpected("Not in a game scene");
  }

  u32 scene_chain[] = {0x80385FC8, 0x54, 0xC};
  u32 scene = TRY(ReadChain(io, scene_chain));
  return ReadFromDolphin<GameScene>(io, scene);
}

struct CourseCache {
  u8 disposer[0x10];
  ptr<void> mpBuffer;
  ptr<void> mpHeap;
  rsl::bu32 mCourseId;
  rsl::bu32 mState;
  ptr<MultiDvdArchive> mpArchive;
};

struct JobContext {
  ptr<MultiDvdArchive> mpMultiArchive;
  ptr<DvdArchive> mpArchive;
  rsl::bu32 _08;
  char mFilename[64];
  ptr<void> mpArchiveHeap;
  ptr<void> mpFileHeap;
};

struct ResourceManager {
  MultiDvdArchive** mppSceneArchives;
  MultiDvdArchive mKartArchives[12];
  MultiDvdArchive mBackupKartArchives[12]; // TODO: better name
  DvdArchive mSystemArchives[4];
  JobContext mJobContexts[7];
  ptr<void> mpTaskThread;
  CourseCache mCourseCache;
#if 0
  MenuCharacterManager mMenuManagers[4];
  bool mIsGlobeLoadingBusy;
  EGG::ExpHeap* mpMenuHeap;
  EGG::Heap* mpGlobeHeap;
  bool mRequestPending;
  bool mRequestsEnabled;
#endif
};

struct KartObjectProxy {
  ptr<void> m_accessor;
  u8 _04[0xc - 0x4];

  Result<u32> getPos(Io io) {
    u32 chain[] = {m_accessor.data + 0x8, 0x90, 0x4};
    auto o = TRY(ReadChain(io, chain));
    if (!o)
      return 0;
    return o + 0x68;
  }
};
static_assert(sizeof(KartObjectProxy) == 0xc);

struct KartObjectManager {
  ptr<void> vtable;
  u8 _04[0x20 - 0x04];
  ptr<ptr<KartObjectProxy>>
      m_objects; // Note: Really a KartObject : KartObjectProxy
  u8 m_count;
  u8 _25[0x38 - 0x25];

  Result<KartObjectProxy> object(Io io, u32 playerId) const {
    return TRY(m_objects.get(io)).get(io, playerId);
  }
};
static_assert(sizeof(KartObjectManager) == 0x38);

} // namespace librii::live_mkw
