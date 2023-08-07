#pragma once

// Utilities to read directly from Mario Kart Wii's memory.
//
// The API is io-agnostic, and should be compatible with RAM dumps, live memory
// or some over-serial debugger.

#include <assert.h>
#include <functional>
#include <optional>
#include <rsl/EnumCast.hpp>
#include <rsl/SimpleReader.hpp>
#include <vendor/magic_enum/magic_enum.hpp>

namespace librii::live_mkw {

// `fread`, essentially
using IoRead = std::function<bool(u32, std::span<u8>)>;
using IoWrite = std::function<bool(u32, std::span<const u8>)>;

struct Io {
  IoRead read = nullptr;
  IoWrite write = nullptr;
};

template <typename T> Result<T> ReadFromDolphin(const Io& io, u32 addr) {
  T raw;
  EXPECT(io.read != nullptr);
  bool ok = io.read(addr, {(u8*)&raw, sizeof(raw)});
  if (!ok) {
    return std::unexpected(
        std::format("Failed to read {} bytes from {:x}", sizeof(raw), addr));
  }
  return raw;
}
template <typename T>
Result<void> WriteToDolphin(const Io& io, u32 addr, const T& obj) {
  EXPECT(io.write != nullptr);
  bool ok = io.write(addr, {reinterpret_cast<const u8*>(&obj), sizeof(obj)});
  if (!ok) {
    return std::unexpected(
        std::format("Failed to write {} bytes to {:x}", sizeof(obj), addr));
  }
  return {};
}

enum class Region {
  P,
  E,
  J,
  K,
};

enum {
  REGION_P_ID = 0x54a9,
  REGION_E_ID = 0x5409,
  REGION_J_ID = 0x53cd,
  REGION_K_ID = 0x5511,
};

Result<Region> GetRegion(Io io) {
  u16 id = TRY(ReadFromDolphin<rsl::bu16>(io, 0x8000620a));
  switch (id) {
  case REGION_P_ID:
    return Region::P;
  case REGION_E_ID:
    return Region::E;
  case REGION_J_ID:
    return Region::J;
  case REGION_K_ID:
    return Region::K;
  default:
    break;
  }
  return std::unexpected("Unknown region ID");
}

constexpr std::array<u32, 4> Symbols[] = {
    {0x809c3618, 0x809bee20, 0x809c2678, 0x809b1c58},
};

//! Port a PAL address to the current region
Result<u32> Port(Io io, u32 addr) {
  auto region = TRY(GetRegion(io));
  if (region == Region::P) {
    return addr;
  }
  if (addr == 0x80385fc8 && region == Region::E) {
    return 0x80381c48;
  }
  if (addr == 0x80385fc8 && region == Region::J) {
    return 0x80385948;
  }
  for (auto& sym : Symbols) {
    if (sym[0] != addr)
      continue;

    u32 index = 0;
    switch (region) {
    case Region::P:
      index = 0;
      break;
    case Region::E:
      index = 1;
      break;
    case Region::J:
      index = 2;
      break;
    case Region::K:
      index = 3;
      break;
    }
    return sym[index];
  }
  return std::unexpected(
      std::format("Region {} is currently unsupported. Please use a PAL disc",
                  magic_enum::enum_name(region)));
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
  u32 scene_id_chain[] = {TRY(Port(io, 0x80385FC8)), 0x54, 0xC, 0x28};
  u32 scene_id = TRY(ReadChain(io, scene_id_chain));
  if (scene_id == 0 || scene_id == 5) {
    return std::unexpected("Not in a game scene");
  }

  u32 scene_chain[] = {TRY(Port(io, 0x80385FC8)), 0x54, 0xC};
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

enum class ItemId {
  None = -0x01,
  Green = 0x00,
  Red = 0x01,
  Nana = 0x02,
  FIB = 0x03,
  Shroom = 0x04,
  TripShrooms = 0x05,
  Bomb = 0x06,
  Blue = 0x07,
  Shock = 0x08,
  Star = 0x09,
  Golden = 0x0a,
  Mega = 0x0b,
  Blooper = 0x0c,
  Pow = 0x0d,
  TC = 0x0e,
  Bill = 0x0f,
  TripGreens = 0x10,
  TripReds = 0x11,
  TripNanas = 0x12,
  Unused = 0x13,
  NoItem = 0x14,
};

// Tentative name
struct KartItem {
  u8 _00[0x88 - 0x00];
  rsl::bs32 _88;
  rsl::bs32 mCurrentItemKind;
  rsl::bs32 mCurrentItemQty;
  u8 _94[0x248 - 0x94];
};
static_assert(sizeof(KartItem) == 0x248);

// Tentative name
struct ItemDirector {
  u8 _00[0x14 - 0x00];
  ptr<KartItem> m_kartItems;
  u8 _48[0x430 - 0x18];
};
static_assert(sizeof(ItemDirector) == 0x430);

static inline u32 ItemDirectorAddr() { return 0x809C3618; }

struct ItemInfo {
  ItemId kind = ItemId::Shroom;
  s32 qty = 1;
};
static inline Result<ItemInfo> GetItem(Io io, u32 playerIndex) {
  u32 idAt = TRY(ReadFromDolphin<rsl::bu32>(io, ItemDirectorAddr()));
  auto itemDir = TRY(ReadFromDolphin<ItemDirector>(io, idAt));
  auto kItem = TRY(itemDir.m_kartItems.get(io, playerIndex));
  auto id = TRY(rsl::enum_cast<ItemId>(kItem.mCurrentItemKind));
  auto qty = static_cast<s32>(kItem.mCurrentItemQty);
  return ItemInfo{
      id,
      qty,
  };
}
static inline Result<ItemInfo> SetItem(Io io, const ItemInfo& info,
                                       u32 playerIndex) {
  u32 idAt = TRY(ReadFromDolphin<rsl::bu32>(io, ItemDirectorAddr()));
  auto itemDir = TRY(ReadFromDolphin<ItemDirector>(io, idAt));
  auto kItem = TRY(itemDir.m_kartItems.get(io, playerIndex));
  kItem.mCurrentItemKind = static_cast<s32>(info.kind);
  kItem.mCurrentItemQty = info.qty;
  std::span<const u8> patch{(const u8*)&kItem.mCurrentItemKind, 8};
  bool ok = io.write(
      itemDir.m_kartItems.data + sizeof(KartItem) * playerIndex + 0x8c, patch);
  if (!ok) {
    return std::unexpected("Failed to write kart item..");
  }
  return {};
}

} // namespace librii::live_mkw
