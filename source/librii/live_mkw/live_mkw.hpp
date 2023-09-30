#pragma once

// Utilities to read directly from Mario Kart Wii's memory.
//
// The API is io-agnostic, and should be compatible with RAM dumps, live memory
// or some over-serial debugger.

#include <assert.h>
#include <functional>
#include <glm/mat4x3.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <rsl/EnumCast.hpp>
#include <rsl/SimpleReader.hpp>
#include <rsl/Utf.hpp>
#include <vendor/magic_enum/magic_enum.hpp>

namespace librii::live_mkw {

// `fread`, essentially
using IoRead = std::function<bool(u32, std::span<u8>)>;
using IoWrite = std::function<bool(u32, std::span<const u8>)>;

struct Io {
  IoRead read = nullptr;
  IoWrite write = nullptr;
};

template <typename T>
static inline Result<T> ReadFromDolphin(const Io& io, u32 addr) {
  T raw;
  memset(&raw, 0xCC, sizeof(raw));
  EXPECT(io.read != nullptr);
  bool ok = io.read(addr, {(u8*)&raw, sizeof(raw)});
  if (!ok) {
    return std::unexpected(
        std::format("Failed to read {} bytes from {:x}", sizeof(raw), addr));
  }
  return raw;
}
template <typename T>
static inline Result<void> WriteToDolphin(const Io& io, u32 addr,
                                          const T& obj) {
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

static inline Result<Region> GetRegion(const Io& io) {
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
static inline Result<u32> Port(const Io& io, u32 addr) {
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
  Result<T> get(const Io& io, u32 idx = 0) const {
    u32 base_addr = static_cast<u32>(data);
    EXPECT(base_addr != 0 && "Nullptr exception");
    u32 elem_addr = base_addr + idx * sizeof(T);
    return ReadFromDolphin<T>(io, elem_addr);
  }
  static ptr<T> at(u32 addr) {
    ptr<T> result;
    result.data = rsl::bu32(addr);
    return result;
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

static inline Result<std::vector<u32>> ReadUtList(const Io& io,
                                                  const ut_List& list) {
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
GameScene_ReadArchives(const Io& io, const GameScene& scn) {
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
static inline Result<u32> ReadChain(const Io& io, std::span<u32> ofs) {
  u32 it = 0;
  for (auto x : ofs) {
    it = TRY(ReadFromDolphin<rsl::bu32>(io, it + x));
    if (it == 0) {
      return 0;
    }
  }
  return it;
}
static inline Result<u32> GetSceneId(const Io& io) {
  u32 scene_id_chain[] = {TRY(Port(io, 0x80385FC8)), 0x54, 0xC, 0x28};
  u32 scene_id = TRY(ReadChain(io, scene_id_chain));
  return scene_id;
}
static inline Result<GameScene> GetGameScene(Io io) {
  u32 scene_id = TRY(GetSceneId(io));
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

struct HitSphere {
  ptr<void> bspEntry;
  rsl::bf32 radius;
  rsl::bu32 _08;
  rsl::bf32 position[3];
  rsl::bf32 last_position[3];
  rsl::bf32 _24[3];
};

struct HitSpheres {
  rsl::bu16 sphere_count;
  rsl::bu16 _02;
  rsl::bu32 bounding_radius;
  char col_data[0x84];
  ptr<HitSphere> spheres;
};
struct KartObjectProxy {
  ptr<void> m_accessor;
  u8 _04[0xc - 0x4];

  Result<u32> getPos(const Io& io) {
    u32 chain[] = {m_accessor.data + 0x8, 0x90, 0x4};
    auto o = TRY(ReadChain(io, chain));
    if (!o)
      return 0;
    return o + 0x68;
  }

  inline Result<HitSpheres> getHitSpheres(const Io& io, u32 player) {
    // player->_00->_08->_90->_08
    u32 hbg_chain[] = {m_accessor.data + 0x8, 0x90, 8};
    u32 hs = TRY(ReadChain(io, hbg_chain));
    return ReadFromDolphin<HitSpheres>(io, hs);
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

  Result<KartObjectProxy> object(const Io& io, u32 playerId) const {
    return TRY(m_objects.get(io)).get(io, playerId);
  }
  u32 numObjects() const { return m_count; }
  static Result<ptr<KartObjectManager>> Get(const Io& io) {
    u32 addr = 0x809c18f8;
    auto ppKOM = TRY(ReadFromDolphin<ptr<KartObjectManager>>(io, addr));
    return ppKOM;
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
static inline Result<ItemInfo> GetItem(const Io& io, u32 playerIndex) {
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
static inline Result<ItemInfo> SetItem(const Io& io, const ItemInfo& info,
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

static inline Result<u32> TryGetPlayerCount(const Io& io) {
  auto pKOM = TRY(KartObjectManager::Get(io));
  auto kom = TRY(pKOM.get(io));
  return kom.numObjects();
}
static inline u32 PlayerCount(const Io& io) {
  return TryGetPlayerCount(io).value_or(0);
}

enum class SectionId {
  None = -1,
  PowerOffWii = 0x0,

  MkChannelInstaller = 0x2,
  WiiMenuRegionSelect = 0x3,
  ReturnToWiiMenu = 0x4,
  Restart = 0x5,
  Thumbnails = 0x6, // Was unused

  CreateSave = 0x10,
  CorruptSave = 0x11,
  CountryChanged = 0x12,
  CannotSave = 0x13,
  CannotAccessNand = 0x14,
  CannotAccessMiis = 0x15,

  ESRBWarning = 0x16,
  FPSWarning1 = 0x17,
  FpsWarning2 = 0x18,

  GPDemo = 0x19,
  VSDemo = 0x1A,
  BTDemo = 0x1B,
  MRBossDemo = 0x1C,
  CoBossDemo = 0x1D,

  GP = 0x1E,
  TA = 0x1F,

  VS1P = 0x20,
  VS2P = 0x21,
  VS3P = 0x22,
  VS4P = 0x23,

  TeamVS1P = 0x24,
  TeamVS2P = 0x25,
  TeamVS3P = 0x26,
  TeamVS4P = 0x27,

  Battle1P = 0x28,
  Battle2P = 0x29,
  Battle3P = 0x2A,
  Battle4P = 0x2B,

  MR = 0x2C,

  TournamentReplay = 0x2D,
  GPReplay = 0x2E,
  TAReplay = 0x2F,

  GhostTA = 0x30,
  GhostTAOnline = 0x31,

  GhostReplayChannel = 0x32,
  GhostReplayDownload = 0x33,
  GhostReplay = 0x34,

  AwardsGP = 0x35,
  AwardsVS = 0x36,
  Unknown37 = 0x37, // Seems to be the same as 0x36
  AwardsBT = 0x38,

  // Unreachable, due to no GP {
  CreditsP1 = 0x39,
  CreditsP1True = 0x3A,
  CreditsP2 = 0x3B,
  CreditsP2True = 0x3C,
  Congratulations = 0x3D,
  CongratulationsTrue = 0x3E,
  // }

  TitleFromBoot = 0x3F,
  TitleFromReset = 0x40,
  TitleFromMenu = 0x41,
  TitleFromNewLicence = 0x42,
  TitleFromOptions = 0x43,

  Demo = 0x44,

  MiiSelectCreate = 0x45,
  MiiSelectChange = 0x46,
  LicenseSettings = 0x47,

  Single = 0x48,
  SingleChangeDriver = 0x49,
  SingleChangeCourse = 0x4A,
  SingleSelectVSCourse = 0x4B,
  SingleSelectBTCourse = 0x4C,
  SingleChangeMission = 0x4D,
  SingleChangeGhostData = 0x4E, // Replaces SingleMkChannelGhost
  SingleChannelLeaderboardChallenge = 0x4F,
  SingleGhostListChallenge = 0x50,

  // From Mario Kart Channel {
  SendGhostDataToFriend = 0x51,
  ChallengeGhost = 0x52,
  WatchReplay = 0x53,
  // }

  Multi = 0x54,

  OnlineSingle = 0x55, // Replaces WifiSingle
  WifiSingleDisconnected = 0x56,
  WifiSingleFriendList = 0x57,
  WifiSingleVsVoting = 0x58,
  WifiSingleBtVoting = 0x59,

  OnlineMultiConfigure = 0x5A, // Replaces WifiMultiMiiConfigure
  OnlineMulti = 0x5B,          // Replaces WifiMulti
  WifiMultiDisconnected = 0x5C,
  WifiMultiFriendList = 0x5D,
  WifiMultiVsVoting = 0x5E,
  WifiMultiBtVoting = 0x5F,

  Voting1PVS = 0x60,
  Voting1PTeamVS = 0x61, // Unused, free to use!
  Voting1PBalloon = 0x62,
  Voting1PCoin = 0x63,
  Voting2PVS = 0x64,
  Voting2PTeamVS = 0x65, // Unused, free to use!
  Voting2PBalloon = 0x66,
  Voting2PCoin = 0x67,

  WifiVS = 0x68,
  WifiMultiVS = 0x69,
  WifiVSSpectate = 0x6A,
  WifiVSMultiSpectate = 0x6B,

  WifiBT = 0x6C,
  WifiMultiBT = 0x6D,
  WifiBTSpectate = 0x6E,
  WifiBTMultiSpectate = 0x6F,

  OnlineFriend1PVS = 0x70,
  OnlineFriend1PTeamVS = 0x71, // Unused, free to use!
  OnlineFriend1PBalloon = 0x72,
  OnlineFriend1PCoin = 0x73,

  OnlineFriend2PVS = 0x74,
  OnlineFriend2PTeamVS = 0x75, // Unused, free to use!
  OnlineFriend2PBalloon = 0x76,
  OnlineFriend2PCoin = 0x77,

  OnlineDisconnected = 0x78,
  OnlineDisconnectedGeneric = 0x79,

  ServicePack = 0x7A,        // Replaces Channel
  ServicePackChannel = 0x7B, // Replaces Channel from Time Trials
  Rankings = 0x7D,
  // ^^ All channel sections vv
  // 0x84 replaces OnlineServer, free to use!

  CompetitionChannel = 0x85,
  CompetitionChangeCharacter = 0x86,
  CompetitionSubmitRecord = 0x87,
  CompetitionWheelOnly = 0x88,
  CompetitionWheelOnlyBosses = 0x89,

  InviteFriend = 0x8A,
  ChannelDownloadData = 0x8B,

  Options = 0x8C,
  AddMkChannel = 0x8D,
  EnableMessageService = 0x8E,

  Unlock0 = 0x90,
  Unlock1 = 0x91,
  Unlock2 = 0x92,
  Unlock3 = 0x93,
  MissionMenu = 0x94,

  Max = 0x95,
  TrialMax = 0xB2,
  // Sections 0x95 - 0xB2 are reserved and cannot be used here

  // Extensions go here {
  // Do not explicitly assign values to prevent merge conflicts
  WU8Library,
  // }
};

struct RegisteredPadManager {
  u8 _00[0x5c - 0x00];
};
static_assert(sizeof(RegisteredPadManager) == 0x5c);
namespace nw4r::lyt {
struct DrawInfo {
  u8 _00[0x54 - 0x00];
};
static_assert(sizeof(DrawInfo) == 0x54);
} // namespace nw4r::lyt
struct Page;

enum class PageId {
  None = -0x1,
  Empty = 0x0,
  EsrbNotice = 0x1,
  FpsNotice = 0x2,
  CorruptSave = 0x3,
  UnableToSave = 0x4,
  CorruptNand = 0x5,
  CorruptMiis = 0x6,
  GpPanOverlay = 0x7,
  VsPanOverlay = 0x8,
  BtPanOverlay = 0x9,
  MrPanOverlay = 0xA,
  ToPanOverlay = 0xB,
  GpHud = 0xC,
  TaHud = 0xD,
  Vs1pHud = 0xE,
  Vs2pHud = 0xF,
  Vs3pHud = 0x10,
  Vs4pHud = 0x11,
  Bt1pHud = 0x12,
  Bt2pHud = 0x13,
  Bt3pHud = 0x14,
  Bt4pHud = 0x15,
  MrToHud = 0x16,
  GpPauseMenu = 0x17,
  VsPauseMenu = 0x18,
  TaPauseMenu = 0x19,
  BattlePauseMenu = 0x1A,
  MrPauseMenu = 0x1B,
  TaGhostPauseMenu = 0x1C,
  Unknown1D =
      0x1D, // "Abandon Ghost Race?" on wiki, cannot find the activation in game
  Unknown1E =
      0x1E, // "Quit Ghost Race?" on wiki, cannot find the activation in game
  GhostReplayPauseMenu = 0x1F,
  AfterGpMenu = 0x20,
  AfterTaMenu = 0x21,
  AfterVsMenu = 0x22,
  AfterBtMenu = 0x23,
  AfterBtFinal = 0x24,
  AfterMrMenu = 0x25,
  AfterToMenu = 0x26,
  AfterGrMenu = 0x27,
  GotoFriendRoster = 0x28,
  SendGhostRecord = 0x29,
  SendTournamentRecord = 0x2A,
  CheckRankings = 0x2B,
  ConfirmQuit = 0x2C,
  AfterTaMenuSplits = 0x2D,
  AfterTaMenuLeaderboard = 0x2E,
  ResultRaceUpdate = 0x2F,
  ResultRaceTotal = 0x30,
  ResultWifiUpdate = 0x31, // Unused
  ResultTeamVSTotal = 0x32,
  ResultBattleUpdate = 0x33,
  ResultBattleTotal = 0x34,
  CompetitionPersonalLeaderboard = 0x35,
  GpReplayHud = 0x36,
  GhostReplayHud = 0x37,
  GpReplayPauseMenu = 0x38,
  TaReplayPauseMenu = 0x39,
  Unknown3A =
      0x3A, // "Supporting Panel, present in many modes in-race, 2nd element"
  AwardRunup = 0x3B,
  AwardInterface = 0x3C,
  Unknown3D = 0x3D,
  Unknown3E = 0x3E,
  CreditsCongrats = 0x3F,

  Wifi1pHud = 0x40,
  Wifi2pHud = 0x41,
  WifiFriendHud = 0x42,
  WifiTeamFriendHud = 0x43,
  Unknown44 = 0x44, // Untested: "Congratuations at end of friend room"
  Unknown45 = 0x45, // "Dummy? Seems to immediately load 0x46"
  AfterWifiMenu = 0x46,
  ConfirmWifiQuit = 0x47,
  OnlinePleaseWait = 0x48,
  Unknown49 = 0x49, // "Live VS view interface"
  Unknown4A = 0x4A, // "Live Battle view interface"
  RaceConfirm = 0x4B,
  SpinnerMessagePopup = 0x4C,
  MessagePopup = 0x4D,
  YesNoPopup = 0x4E,
  SpinnerAwait = 0x4F,
  ConnectingNintendoWfc = 0x50,
  MenuMessage = 0x51,
  Confirm = 0x52,
  MessageBoardPopup = 0x53,
  Unknown54 = 0x54, // "Behind main menu?" however isn't activated on main menu.
  LowBatteryPopupManager = 0x55,
  LowBatteryPopup = 0x56,
  Title = 0x57,
  Unknown58 = 0x58, // "Behind main menu?" however isn't activated on main menu.
  OpeningMovie = 0x59,
  TopMenu = 0x5A,
  Unknown5B = 0x5B, // "Behind unlocks?"
  Model = 0x5C,
  LineBackgroundWhite = 0x5D,
  Obi = 0x5E,
  PressA = 0x5F,
  SelectMii = 0x60,
  PlayerPad = 0x61, // Activates ControllerRegister when activated
  ControllerRegister = 0x62,
  ControllerRegisterInstructions = 0x63,
  ControllerRegisterComplete = 0x64,
  LicenseSelect = 0x65,
  LicenceDisplay = 0x66,
  LicenseSettingsTop = 0x67,
  LicenceEraseConfirm = 0x68,
  SingleTop = 0x69,
  GpClassSelect = 0x6A,
  CharacterSelect = 0x6B,
  VehicleSelect = 0x6C,
  DriftSelect = 0x6D,
  CourseSelect = 0x6E, // Replaces RaceCupSelect
  // Disabled {
  RaceCourseSelect = 0x6F,
  // }
  TimeAttackTop = 0x70,
  TimeAttackGhostList = 0x71,
  VSSelect = 0x72, // Unused
  VSSetting = 0x73,
  TeamConfirm = 0x74,
  BattleModeSelect = 0x75,
  BattleVehicleSelect = 0x76,
  BattleSetting = 0x77,
  // Disabled {
  BattleCupSelect = 0x78,
  BattleCourseSelect = 0x79,
  // }
  MissionLevelSelect = 0x7a,
  MissionStageSelect = 0x7b,
  MissionInstruction = 0x7c,
  MissionDrift = 0x7d,
  MissionTutorial = 0x7e,
  ModelRender = 0x7f,
  MultiTop = 0x80,
  MultiVehicleSelect = 0x81,
  MultiDriftSelect = 0x82,
  MultiTeamSelect = 0x83,
  DirectConnection = 0x84, // Replaces WifiConnect (or something)
  WifiFirstPlay = 0x85,
  WifiDataConsent = 0x86,
  WifiDisconnect = 0x87,          // "Disconnects you"
  OnlineConnectionManager = 0x88, // Replaces unknown page
  WifiConnectionFailed = 0x89,
  WifiMultiConfirm = 0x8A,
  OnlineTop = 0x8B,        // Replaces WifiTop
  OnlineModeSelect = 0x8C, // Replaces OnlineModeSelect
  WifiFriendMenu = 0x8D,
  OnlineTeamSelect = 0x8E, // Replaces MkChannelFriendMenu
  RandomMatching = 0x8F,   // Replaces Global Search Manager
  VotingBack = 0x90,       // Replaces CountDownTimer
  WifiPlayerList = 0x91,
  Roulette = 0x92,
  Unknown93 = 0x93, // "Present in live view?"
  Unknown94 = 0x94, // "Present in online race?"
  Globe = 0x95,
  WifiFriendRoster = 0x96,
  WifiNoFriendsPopup = 0x97,
  WifiFriendRemoveConfirm = 0x98,
  WifiFriendRemoving = 0x99,
  FriendRoomRules = 0x9A, // Replaces FriendJoin
  FriendMatching = 0x9B,  // Replaces "waiting" text
  FriendRoomBack = 0x9C,  // Replaces Friend Room Manager
  FriendRoom = 0x9D,
  FriendRoomMessageSelect = 0x9E,

  ServicePackTop = 0xA2,   // Replaces ChannelTop
  StorageBenchmark = 0xA3, // Replaces ChannelRanking
  ServicePackTools = 0xA4, // Replaces ChannelGhost
  UnknownA5 = 0xA5,        // "Dummy? Seems to redirect to 0xA6"
  EnterFriendCode = 0xA6,
  GhostManager = 0xA7,
  Ranking = 0xA8,
  RankingBack = 0xAA,

  ChannelGhostBackground = 0xAC,
  ChannelGhostSelected = 0xAD,
  RankingTopTenDownload = 0xAE,

  UnknownB3 = 0xB3, // "Resides behind 0x4F, loads 0xB4"
  ChannelGhostList = 0xB4,
  ChannelGhostErase = 0xB5,

  CompetitionWheelOnly = 0xBA,

  Options = 0xC0,
  WifiOptions = 0xC1,
  OptionExplanation = 0xC2,
  OptionSelect2 = 0xC3,
  OptionSelect3 = 0xC4,
  RegionOption = 0xC5,
  RegionDisplayOption = 0xC6,
  OptionAwait = 0xC7,
  OptionMessage = 0xC8,
  OptionConfirm = 0xC9,
  Channel = 0xCA, // Replaces ChannelExplanation
  Update = 0xCB,  // Replaces ChannelConfirm
  OptionsBackground = 0xCC,

  MenuSettings = 0xCE,  // Replaces LicenseRecordsOverall
  SettingsPopup = 0xCF, // Replaces LicenseRecordsFavorites
  // Disabled {
  ServicePackChannel = 0xD0, // Replaces LicenseRecordsFriends
  LicenseRecordsWFC = 0xD1,
  LicenseRecordsOther = 0xD2,
  // }

  Max = 0xD3,

  Ext_MinExclusive__ = 0xFF,

  // Extensions go here {
  // Do not explicitly assign values to prevent merge conflicts
  PackSelect,
  CourseDebug,
  WU8Library,
  RankingDownloadManager,
  // }

  Ext_MaxExclusive__,
};

static_assert(static_cast<size_t>(PageId::Ext_MaxExclusive__) >
              static_cast<size_t>(PageId::Ext_MinExclusive__));

constexpr size_t StandardPageCount() {
  return static_cast<size_t>(PageId::Max);
}

constexpr size_t ExtendedPageCount() {
  return static_cast<size_t>(PageId::Ext_MaxExclusive__) -
         static_cast<size_t>(PageId::Ext_MinExclusive__) - 1;
}

struct Section {
  rsl::bs32 m_id;
  u8 _004[0x008 - 0x004];
  std::array<ptr<Page>, StandardPageCount()> m_pages;
  ptr<Page> m_activePages[10];
  rsl::bu32 m_activePageCount;
  std::array<ptr<Page>, 2> m_systemPages;
  u8 _388[0x390 - 0x388];
  nw4r::lyt::DrawInfo m_drawInfo;
  u8 _3e4[0x3f8 - 0x3e4];
  rsl::bf32 m_scaleFor[2];
  u8 _400[0x408 - 0x400];
};
static_assert(sizeof(Section) == 0x408);
enum class OnlineErrorCategory {
  ErrorCode = 0x1,
  MiiInvalid = 0x2,
  GeneralDisconnect = 0x4,
  UnrecoverableDisconnect = 0x5,
};

static inline Result<std::string>
ReadName(std::span<const rsl::endian_swapped_value<u16>> str) {
  std::array<u16, 10> little_endian{};
  for (size_t i = 0; i < 10; ++i) {
    little_endian[i] = str[i];
  }
  // Convert UTF-16 to UTF-32
  return rsl::StdStringFromUTF16(little_endian);
}

namespace System {

struct RawMii;

struct MiiId {
  u8 avatar[4];
  u8 client[4];
};
static_assert(sizeof(MiiId) == 0x8);

struct RawMii {
  u8 _00[0x02 - 0x00];
  std::array<rsl::endian_swapped_value<u16>, 10> name;
  u8 _16[0x4a - 0x16];
  rsl::bu16 crc16;

  Result<std::string> getName() const { return ReadName(name); }
};
static_assert(sizeof(RawMii) == 0x4c);

struct Mii {
  u8 _00[0x10 - 0x00];
  RawMii m_raw;
  u8 _5c[0x68 - 0x5c];
  std::array<rsl::endian_swapped_value<u16>, 10> name;
  u8 _7c[0x94 - 0x7c];
  MiiId m_id;
  u8 _9c[0xb8 - 0x9c];

  Result<std::string> getName() const { return ReadName(name); }
};
static_assert(sizeof(Mii) == 0xb8);

} // namespace System

struct MiiGroup {
  struct Texture {
    u8 _00[0x24 - 0x00];
  };
  static_assert(sizeof(Texture) == 0x24);

  ptr<void> vtable;
  ptr<ptr<System::Mii>> m_miis;
  ptr<Texture> m_textures[7];
  u8 _24[0x28 - 0x24];
  rsl::bu32 m_miiCount;
  u8 _2C[0x98 - 0x2C];

  Result<std::string> GetMiiName(const Io& io, size_t index) {
    EXPECT(index < m_miiCount && "Invalid mii ID");
    auto arr = TRY(m_miis.get(io));
    auto mii = TRY(arr.get(io, index));
    return mii.getName();
  }
};
static_assert(sizeof(MiiGroup) == 0x98);

struct GlobalContext {
  struct SelectPlayer {
    rsl::bu32 m_characterId;
    rsl::bu32 m_vehicleId;
    u8 _08;
  };

  struct OnlineDisconnectInfo {
    rsl::bs32 m_category;
    rsl::bu32 m_errorCode;
  };

  enum class VehicleRestriction : u32 {
    KartsOnly = 0,
    BikesOnly = 1,
    All = 2,
  };

  u8 _000[0x060 - 0x000];
  rsl::bu32 m_match;
  rsl::bu32 m_matchCount;
  u8 _068[0x074 - 0x068];
  rsl::bs32 m_vehicleRestriction;

  // Unused as replaced {
  std::array<rsl::bs32, 32> m_unusedRaceOrder;
  rsl::bu32 m_unusedOrderCount; // Always 32, as equal to courses unlocked
  std::array<rsl::bs32, 10> m_unusedBattleOrder;
  // }
  rsl::bu32 m_localPlayerCount;
  u8 _128[0x12c - 0x128];
  rsl::bs32 m_localCharacterIds[4];
  rsl::bs32 m_localVehicleIds[4];
  rsl::bu32 m_raceCourseId;
  rsl::bu32 m_battleCourseId;
  u8 _154[0x164 - 0x154];
  rsl::bu32 m_driftModes[4];
  u8 _174[0x188 - 0x174];
  MiiGroup m_playerMiis;
  SelectPlayer m_selectPlayer[2];
  MiiGroup m_localPlayerMiis;
  u8 _2d0[0x3bc - 0x2d0];
  rsl::bu32 _3bc;
  u8 _3c0[0x3c4 - 0x3c0];
  rsl::bu32 m_timeAttackGhostType;
  rsl::bs32 m_timeAttackCourseId;
  rsl::bs32 m_timeAttackLicenseId;
  u8 _3d0[0x4c8 - 0x3d0];
  rsl::bu32 m_lastTitleBackground;
  u8 _4cc[0x500 - 0x4cc];
  OnlineDisconnectInfo m_onlineDisconnectInfo;
  u8 _508[0x510 - 0x508];
};
static_assert(sizeof(GlobalContext) == 0x510);

struct SectionManager {
  ptr<Section> m_currentSection;
  u8 _04[0x0c - 0x04];
  rsl::bu32 m_nextSectionId;
  rsl::bu32 m_lastSectionId;
  rsl::bu32 m_currentAnimDir;
  rsl::bu32 m_nextAnimDir;
  rsl::bu32 m_changeTimer;
  bool m_firstLoad;
  u8 _21[0x2C - 0x21];
  rsl::bs32 m_transitionFrame;
  rsl::bu32 m_state;
  RegisteredPadManager m_registeredPadManager;
  ptr<void> m_saveManagerProxy;
  u8 _94[0x98 - 0x94];
  ptr<GlobalContext> m_globalContext;

  Result<SectionId> curSectionId(const Io& io) {
    s32 id = TRY(m_currentSection.get(io)).m_id;
    // TODO: rsl::enum_cast
    return static_cast<SectionId>(id);
  }

  static Result<ptr<SectionManager>> Get(const Io& io) {
    return ReadFromDolphin<ptr<SectionManager>>(io, 0x809c1e38);
  }
};
static_assert(sizeof(SectionManager) == 0x9c);

struct Struct_0x6c {
  rsl::bu32 _00;
  std::array<rsl::bf32, 3 * 4> viewMtx;
};
struct Struct_0x10 {
  char _[0x6c];
  ptr<Struct_0x6c> _6C;
};
struct SubRenderManager {
  char _[0x10];
  ptr<Struct_0x10> _10;

  static inline ptr<ptr<SubRenderManager>> s_inst =
      ptr<ptr<SubRenderManager>>::at(0x809C1830);
};

static inline Result<SectionId> GetSectionId(const Io& io) {
  auto pSM = TRY(SectionManager::Get(io));
  auto sm = TRY(pSM.get(io));
  auto id = TRY(sm.curSectionId(io));
  return id;
}

static inline Result<std::string> GetMiiName(const Io& io, size_t index) {
  auto pSM = TRY(SectionManager::Get(io));
  auto sectionManager = TRY(pSM.get(io));
  auto global = TRY(sectionManager.m_globalContext.get(io));
  auto name = TRY(global.m_playerMiis.GetMiiName(io, index));
  return name;
}

static inline Result<glm::mat4x3> GetViewMatrix(const Io& io) {
  auto pSRM = TRY(SubRenderManager::s_inst.get(io));
  auto srm = TRY(pSRM.get(io));
  auto s10 = TRY(srm._10.get(io));
  auto s6c = TRY(s10._6C.get(io));
  auto bigEndianRowMajor = s6c.viewMtx;
  glm::mat4x3 result;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j][i] = bigEndianRowMajor[i * 4 + j];
    }
  }
  return result;
}

struct HSInfo {
  glm::vec3 pos{};
  f32 radius{};
};
static inline Result<std::vector<HSInfo>> GetSpheres(const Io& io) {
  std::vector<HSInfo> result;
  auto pKOM = TRY(KartObjectManager::Get(io));
  auto kom = TRY(pKOM.get(io));
  u32 count = kom.numObjects();
  for (size_t i = 0; i < count; ++i) {
    auto ko = TRY(kom.object(io, i));
    auto hs = TRY(ko.getHitSpheres(io, 0));
    u32 nhs = hs.sphere_count;
    for (size_t j = 0; j < nhs; ++j) {
      auto s = TRY(hs.spheres.get(io, j));
      glm::vec3 pos(s.position[0], s.position[1], s.position[2]);
      f32 radius = s.radius;
      result.push_back(HSInfo{
          .pos = pos,
          .radius = radius,
      });
    }
  }
  return result;
}

} // namespace librii::live_mkw
