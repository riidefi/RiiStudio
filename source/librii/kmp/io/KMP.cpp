#include "KMP.hpp"
#include <core/util/glm_io.hpp>
#include <llvm/ADT/StringMap.h>
#include <rsl/SafeReader.hpp>

namespace librii::kmp {

struct IOContext {
  std::string path;

  auto sublet(const std::string& dir) { return IOContext(path + "/" + dir); }

  // void callback(kpi::IOMessageClass mclass, const std::string_view message) {
  //   transaction.callback(mclass, path, message);
  // }

  void inform(const std::string_view message) {
    // callback(kpi::IOMessageClass::Information, message);
  }
  void warn(const std::string_view message) {
    // callback(kpi::IOMessageClass::Warning, message);
  }
  void error(const std::string_view message) {
    // callback(kpi::IOMessageClass::Error, message);
  }
  void request(bool cond, const std::string_view message) {
    if (!cond)
      warn(message);
  }
  void require(bool cond, const std::string_view message) {
    if (!cond)
      error(message);
  }

  IOContext(std::string&& p) : path(std::move(p)) {}
};

Result<CourseMap> readKMP(std::span<const u8> data) {
  CourseMap map;
  oishii::BinaryReader reader(data, "Unknown path");
  rsl::SafeReader safe(reader);
  IOContext ctx("kmp");

  u16 num_sec = 0;
  u16 header_size = 0;

  {
    auto g = ctx.sublet("header");

    g.require(TRY(reader.tryRead<u32>()) == 'RKMD', "Invalid file magic");
    g.require(reader.tryRead<u32>() == reader.endpos(), "Invalid file size");

    num_sec = TRY(reader.tryRead<u16>());
    g.request(num_sec == 15, "Unusual section count");

    header_size = TRY(reader.tryRead<u16>());
    g.require(header_size == 0x10 + num_sec * 4, "Spoopy header length");

    map.mRevision = TRY(reader.tryRead<u32>());
    g.require(map.mRevision == 2520, "Unusual revision");
  }

  EXPECT(num_sec < 32);
  u32 sections_handled = 0;

  const auto search = [&](u32 key,
                          bool standard_fields =
                              true) -> Result<std::tuple<bool, u16, u16>> {
    for (u32 i = 0; i < num_sec; ++i) {
      reader.seekSet(
          header_size +
          reader.tryGetAt<u32>(header_size + (i - num_sec) * 4).value());
      const auto magic = TRY(reader.tryRead<u32>());
      if (magic == key) {
        sections_handled |= (1 << i);
        if (!standard_fields)
          return std::tuple<bool, u16, u16>{true, 1, 0};
        const auto num_entry = TRY(reader.tryRead<u16>());
        const auto user_data = TRY(reader.tryRead<u16>());
        return std::tuple<bool, u16, u16>{true, num_entry, user_data};
      }
    }
    return std::tuple<bool, u16, u16>{false, 0, 0};
  };

  if (auto [found, num_entry, user_data] =
          TRY(search('KTPT', map.mRevision > 1830));
      found) {
    map.mStartPoints.resize(num_entry);
    for (auto& entry : map.mStartPoints) {
      entry.position << reader;
      entry.rotation << reader;
      entry.player_index = TRY(reader.tryRead<s16>());
      entry._ = TRY(reader.tryRead<u16>());
    }
  }

  const auto read_path_section = [&](u32 path_key, u32 point_key, auto& sec,
                                     u32 point_stride,
                                     auto read_point) -> Result<void> {
    auto [pt_found, pt_num_entry, pt_user_data] = TRY(search(point_key));
    if (!pt_found)
      return {};
    const auto pt_ofs = reader.tell();

    auto [ph_found, ph_num_entry, ph_user_data] = TRY(search(path_key));
    if (!ph_found)
      return {};

    sec.resize(ph_num_entry);
    for (auto& entry : sec) {
      const u8 start = TRY(reader.tryRead<u8>());
      const u8 size = TRY(reader.tryRead<u8>());

      for (auto& e : TRY(reader.tryReadX<u8, 6>()))
        if (e != 0xFF)
          entry.mPredecessors.push_back(e);
      for (auto& e : TRY(reader.tryReadX<u8, 6>()))
        if (e != 0xFF)
          entry.mSuccessors.push_back(e);

      entry.misc = TRY(reader.tryReadX<u8, 2>());

      {
        oishii::Jump<oishii::Whence::Set> g(reader,
                                            pt_ofs + point_stride * start);
        entry.points.resize(size);
        for (auto& pt : entry.points)
          read_point(pt, reader);
      }
    }
    return {};
  };

  const auto read_enpt = [](EnemyPoint& pt,
                            oishii::BinaryReader& reader) -> Result<void> {
    pt.position << reader;
    pt.deviation = TRY(reader.tryRead<f32>());
    pt.param = TRY(reader.tryReadX<u8, 4>());
    return {};
  };
  TRY(read_path_section('ENPH', 'ENPT', map.mEnemyPaths, 0x14, read_enpt));

  const auto read_itpt = [](ItemPoint& pt,
                            oishii::BinaryReader& reader) -> Result<void> {
    pt.position << reader;
    pt.deviation = TRY(reader.tryRead<f32>());
    pt.param = TRY(reader.tryReadX<u8, 4>());
    return {};
  };
  TRY(read_path_section('ITPH', 'ITPT', map.mItemPaths, 0x14, read_itpt));

  const auto read_ckpt = [&](CheckPoint& pt,
                             oishii::BinaryReader& reader) -> Result<void> {
    pt.mLeft << reader;
    pt.mRight << reader;
    pt.mRespawnIndex = TRY(reader.tryRead<u8>());
    pt.mLapCheck = TRY(reader.tryRead<u8>());
    // TODO: We assume the intrusive linked-list data is valid
    reader.skip(2);
    return {};
  };
  TRY(read_path_section('CKPH', 'CKPT', map.mCheckPaths, 0x14, read_ckpt));

  if (auto [found, num_entry, user_data] = TRY(search('GOBJ')); found) {
    map.mGeoObjs.resize(num_entry);
    for (auto& entry : map.mGeoObjs) {
      entry.id = TRY(reader.tryRead<u16>());
      entry._ = TRY(reader.tryRead<u16>());
      entry.position << reader;
      entry.rotation << reader;
      entry.scale << reader;
      entry.pathId = TRY(reader.tryRead<u16>());
      entry.settings = TRY(reader.tryReadX<u16, 8>());
      entry.flags = TRY(reader.tryRead<u16>());
    }
  }

  if (auto [found, num_entry, user_data] = TRY(search('POTI')); found) {
    const auto total_points_expected = user_data;
    u32 total_points_real = 0;

    map.mPaths.resize(num_entry);
    for (auto& entry : map.mPaths) {
      const auto entry_size = TRY(reader.tryRead<u16>());
      entry.points.resize(entry_size);
      total_points_real += entry_size;
      entry.interpolation = TRY(safe.Enum8<Interpolation>());
      entry.loopPolicy = TRY(safe.Enum8<LoopPolicy>());
      for (auto& sub : entry.points) {
        sub.position << reader;
        sub.params = TRY(reader.tryReadX<u16, 2>());
      }
    }

    ctx.sublet("POTI").require(total_points_expected == total_points_real,
                               "Expected " +
                                   std::to_string(total_points_expected) +
                                   " POTI points. Instead saw " +
                                   std::to_string(total_points_real) + ".");
  }

  if (auto [found, num_entry, user_data] = TRY(search('AREA')); found) {
    auto area_ctx = ctx.sublet("Areas");
    map.mAreas.resize(num_entry);

    int i = 0;
    for (auto& entry : map.mAreas) {
      auto entry_ctx = area_ctx.sublet("#" + std::to_string(i++));

      auto raw_area_shp = TRY(reader.tryRead<u8>());
      if (raw_area_shp > 1) {
        entry_ctx.error("Invalid area shape: " + std::to_string(raw_area_shp) +
                        ". Expected range: [0, 1]. Defaulting to 0 (Box).");
        raw_area_shp = 0;
      }

      auto raw_area_type = TRY(reader.tryRead<u8>());
      if (raw_area_type > 10) {
        entry_ctx.error(
            "Invalid area type: " + std::to_string(raw_area_type) +
            ". Expected range: [0, 10]. This will be ignored  by the game.");
      }

      entry.getModel().mShape = static_cast<AreaShape>(raw_area_shp);
      entry.mType = static_cast<AreaType>(raw_area_type);
      entry.mCameraIndex = TRY(reader.tryRead<u8>());
      entry.mPriority = TRY(reader.tryRead<u8>());
      entry.getModel().mPosition << reader;
      entry.getModel().mRotation << reader;
      entry.getModel().mScaling << reader;

      entry.mParameters = TRY(reader.tryReadX<u16, 2>());
      if (map.mRevision >= 2200) {
        entry.mRailID = TRY(reader.tryRead<u8>());
        entry.mEnemyLinkID = TRY(reader.tryRead<u8>());
        entry.mPad = TRY(reader.tryReadX<u8, 2>());
      } else {
        entry.mRailID = 0xFF;
        entry.mEnemyLinkID = 0xFF;
        entry.mPad = {};
      }
    }
  }

  if (auto [found, num_entry, user_data] = TRY(search('CAME')); found) {
    map.mOpeningPanIndex = user_data >> 8;
    map.mVideoPanIndex = user_data & 0xff;
    map.mCameras.resize(num_entry);
    for (auto& entry : map.mCameras) {
      entry.mType = TRY(safe.Enum8<CameraType>());
      entry.mNext = TRY(reader.tryRead<u8>());
      entry.mShake = TRY(reader.tryRead<u8>());
      entry.mPathId = TRY(reader.tryRead<u8>());
      entry.mPathSpeed = TRY(reader.tryRead<u16>());
      entry.mFov.mSpeed = TRY(reader.tryRead<u16>());
      entry.mView.mSpeed = TRY(reader.tryRead<u16>());
      entry.mStartFlag = TRY(reader.tryRead<u8>());
      entry.mMovieFlag = TRY(reader.tryRead<u8>());
      entry.mPosition << reader;
      entry.mRotation << reader;
      entry.mFov.from = TRY(reader.tryRead<f32>());
      entry.mFov.to = TRY(reader.tryRead<f32>());
      entry.mView.from << reader;
      entry.mView.to << reader;
      entry.mActiveFrames = TRY(reader.tryRead<f32>());
    }
  }

  if (auto [found, num_entry, user_data] = TRY(search('JGPT')); found) {
    map.mRespawnPoints.resize(num_entry);
    for (auto& entry : map.mRespawnPoints) {
      entry.position << reader;
      entry.rotation << reader;
      entry.id = TRY(reader.tryRead<u16>());
      entry.range = TRY(reader.tryRead<u16>());
    }
  }

  if (auto [found, num_entry, user_data] = TRY(search('CNPT')); found) {
    auto ctx_cnpt = ctx.sublet("Cannons");

    map.mCannonPoints.resize(num_entry);
    int i = 0;
    for (auto& entry : map.mCannonPoints) {
      auto ctx_entry = ctx_cnpt.sublet(std::to_string(i));
      entry.mPosition << reader;
      entry.mRotation << reader;
      ctx_entry.require(TRY(reader.tryRead<u16>()) == i, "Invalid cannon ID");
      entry.mType = TRY(safe.Enum8<CannonType>());
      ++i;
    }
  }

  if (auto [found, num_entry, user_data] = TRY(search('MSPT')); found) {
    map.mMissionPoints.resize(num_entry);
    for (auto& entry : map.mMissionPoints) {
      entry.position << reader;
      entry.rotation << reader;
      entry.id = TRY(reader.tryRead<u16>());
      entry.unknown = TRY(reader.tryRead<u16>());
    }
  }

  if (auto [found, num_entry, user_data] = TRY(search('STGI')); found) {
    map.mStages.resize(num_entry);
    for (auto& entry : map.mStages) {
      entry.mLapCount = TRY(reader.tryRead<u8>());
      entry.mCorner = TRY(safe.Enum8<Corner>());
      entry.mStartPosition = TRY(safe.Enum8<StartPosition>());
      entry.mFlareTobi = TRY(reader.tryRead<u8>());
      entry.mLensFlareOptions.a = TRY(safe.U8());
      entry.mLensFlareOptions.r = TRY(safe.U8());
      entry.mLensFlareOptions.g = TRY(safe.U8());
      entry.mLensFlareOptions.b = TRY(safe.U8());

      if (map.mRevision >= 2320) {
        entry.mUnk08 = TRY(safe.U8());
        entry._ = TRY(safe.U8());
        entry.mSpeedModifier = TRY(safe.U16());
      } else {
        entry.mUnk08 = 0;
        entry._ = 0;
        entry.mSpeedModifier = 0;
      }
    }
  }

  return map;
}

class RelocWriter {
public:
  struct Reloc {
    std::string from, to;
    std::size_t ofs, sz;
  };

  RelocWriter(oishii::Writer& writer) : mWriter(writer) {}
  // Define a label, associated with the current stream position
  void label(const std::string& id) { mLabels.try_emplace(id, mWriter.tell()); }
  // Write a relocation, to be filled in by a resolve() call
  template <typename T>
  void writeReloc(const std::string& from, const std::string& to) {
    mRelocs.emplace_back(
        Reloc{.from = from, .to = to, .ofs = mWriter.tell(), .sz = sizeof(T)});
    mWriter.write<T>(static_cast<T>(-1));
  }
  // Write a relocation, with an associated child frame to be written by
  // writeChildren()
  template <typename T>
  void writeReloc(const std::string& from, const std::string& to,
                  std::function<void(oishii::Writer&)> write) {
    mRelocs.emplace_back(
        Reloc{.from = from, .to = to, .ofs = mWriter.tell(), .sz = sizeof(T)});
    mWriter.write<T>(static_cast<T>(-1));
    mChildren.emplace_back(to, write);
  }
  // Write all child bodies
  void writeChildren() {
    for (auto& child : mChildren) {
      label(child.first);
      child.second(mWriter);
    }
    mChildren.clear();
  }
  // Resolve a single relocation
  void resolve(Reloc& reloc) {
    const auto from = mLabels.find(reloc.from);
    const auto to = mLabels.find(reloc.to);

    std::size_t delta = 0;
    if (from == mLabels.end() || to == mLabels.end()) {
      printf("Bad lookup: %s to %s\n", reloc.from.c_str(), reloc.to.c_str());
    } else {
      delta = to->second - from->second;
    }

    const auto back = mWriter.tell();

    mWriter.seekSet(reloc.ofs);

    if (reloc.sz == 4) {
      mWriter.write<u32>(delta);
    } else if (reloc.sz == 2) {
      mWriter.write<u16>(delta);
    } else {
      assert(false);
      printf("Invalid reloc size..\n");
    }

    mWriter.seekSet(back);
  }
  // Resolve all relocations
  void resolve() {
    for (auto& reloc : mRelocs)
      resolve(reloc);
  }

private:
  oishii::Writer& mWriter;
  std::map<std::string, std::size_t> mLabels;

  rsl::small_vector<Reloc, 64> mRelocs;
  rsl::small_vector<
      std::pair<std::string, std::function<void(oishii::Writer&)>>, 16>
      mChildren;
};

std::string stringifyId(u32 key) {
  const char fmt[5]{static_cast<char>((key & 0xFF00'0000) >> 24),
                    static_cast<char>((key & 0x00FF'0000) >> 16),
                    static_cast<char>((key & 0x0000'FF00) >> 8),
                    static_cast<char>(key & 0x0000'00FF), '\0'};
  return fmt;
}

void writeKMP(const CourseMap& map, oishii::Writer& writer) {
  writer.setEndian(std::endian::big);
  RelocWriter reloc(writer);
  reloc.label("KMP_BEGIN");

  writer.write<u32>('RKMD');
  reloc.writeReloc<u32>("KMP_BEGIN", "KMP_END");
  writer.write<u16>(15);                      // 15 sections
  writer.write<u16>(15 * sizeof(u32) + 0x10); // header size
  writer.write<u32>(map.mRevision);

  reloc.writeReloc<s32>("KMP", "KTPT", [&](oishii::Writer& stream) {
    stream.write<u32>('KTPT');
    if (map.mRevision > 1830) {
      stream.write<u16>(map.mStartPoints.size());
      stream.write<u16>(0); // user data
    }
    for (auto& point : map.mStartPoints) {
      point.position >> writer;
      point.rotation >> writer;
      stream.write<u16>(point.player_index);
      stream.write<u16>(point._);
    }
  });
  // All points are written contiguously by order of their path
  // No duplicate removal
  const auto write_path = [&]<typename T = float>(
      u32 ph_key, u32 pt_key, auto paths, auto write_point, T = {}) {

    auto point_impl = [](oishii::Writer& stream, u32 pt_key, auto paths,
                         auto write_point) {
      stream.write<u32>(pt_key);
      stream.write<u16>(std::accumulate(
          paths.begin(), paths.end(), 0,
          [](auto total, auto& path) { return total + path.points.size(); }));
      stream.write<u16>(0); // user data

      std::size_t i = 0;
      for (auto& path : paths) {
        const auto p_start = i;
        for (auto& point : path.points) {
          if constexpr (std::is_same_v<T, bool>) {
            write_point(point, stream, i, p_start,
                        p_start + path.points.size());
          } else {
            write_point(point, stream);
          }
          ++i;
        }
      }
    };
    auto path_impl = [](oishii::Writer& stream, u32 pt_key, auto paths) {
      stream.write<u32>(pt_key);
      stream.write<u16>(paths.size());
      stream.write<u16>(0); // user data
      u8 ph_start_index = 0;
      for (auto& path : paths) {
        stream.write<u8>(ph_start_index);
        ph_start_index += path.points.size();
        stream.write<u8>(path.points.size());
        for (auto p : path.mPredecessors)
          stream.write<u8>(p);
        for (int i = path.mPredecessors.size(); i < 6; ++i)
          stream.write<u8>(0xff);
        for (auto p : path.mSuccessors)
          stream.write<u8>(p);
        for (int i = path.mSuccessors.size(); i < 6; ++i)
          stream.write<u8>(0xff);
        for (auto p : path.misc)
          stream.write<u8>(p);
      }
    };

    reloc.writeReloc<s32>("KMP", stringifyId(pt_key),
                          [=](oishii::Writer& writer) {
                            point_impl(writer, pt_key, paths, write_point);
                          });

    reloc.writeReloc<s32>(
        "KMP", stringifyId(ph_key),
        [=](oishii::Writer& writer) { path_impl(writer, ph_key, paths); });
  };

  const auto write_enpt = [](const EnemyPoint& point, oishii::Writer& writer) {
    assert(&writer);
    point.position >> writer;
    writer.write<f32>(point.deviation);
    for (auto p : point.param)
      writer.write<u8>(p);
  };
  write_path('ENPH', 'ENPT', map.mEnemyPaths, write_enpt);

  const auto write_itpt = [](const ItemPoint& point, oishii::Writer& writer) {
    point.position >> writer;
    writer.write<f32>(point.deviation);
    for (auto p : point.param)
      writer.write<u8>(p);
  };
  write_path('ITPH', 'ITPT', map.mItemPaths, write_itpt);

  const auto write_ckpt = [](const CheckPoint& point, oishii::Writer& writer,
                             std::size_t seq, std::size_t first,
                             std::size_t last) {
    point.mLeft >> writer;
    point.mRight >> writer;
    writer.write<u8>(point.mRespawnIndex);
    writer.write<u8>(point.mLapCheck);
    // Last, Next
    writer.write<u8>(seq <= first ? 0xFF : seq - 1);
    writer.write<u8>(seq + 1 == last ? 0xFF : seq + 1);
  };
  write_path('CKPH', 'CKPT', map.mCheckPaths, write_ckpt, true);

  reloc.writeReloc<s32>("KMP", "GOBJ", [&](oishii::Writer& stream) {
    stream.write<u32>('GOBJ');
    stream.write<u16>(map.mGeoObjs.size());
    stream.write<u16>(0); // user data
    for (auto& entry : map.mGeoObjs) {
      stream.write<u16>(entry.id);
      stream.write<u16>(entry._);
      entry.position >> stream;
      entry.rotation >> stream;
      entry.scale >> stream;
      stream.write<u16>(entry.pathId);
      for (auto s : entry.settings)
        stream.write<u16>(s);
      stream.write<u16>(entry.flags);
    }
  });

  reloc.writeReloc<s32>("KMP", "POTI", [&](oishii::Writer& stream) {
    stream.write<u32>('POTI');
    stream.write<u16>(map.mPaths.size());
    u32 total_count = 0;
    for (auto& entry : map.mPaths)
      total_count += entry.points.size();
    stream.write<u16>(total_count);
    for (auto& entry : map.mPaths) {
      stream.write<u16>(entry.points.size());
      stream.write<u8>(static_cast<u8>(entry.interpolation));
      stream.write<u8>(static_cast<u8>(entry.loopPolicy));
      for (auto& sub : entry.points) {
        sub.position >> stream;
        for (auto p : sub.params)
          stream.write<u16>(p);
      }
    }
  });

  reloc.writeReloc<s32>("KMP", "AREA", [&](oishii::Writer& stream) {
    stream.write<u32>('AREA');
    stream.write<u16>(map.mAreas.size());
    stream.write<u16>(0); // user data
    for (auto& entry : map.mAreas) {
      stream.write<u8>(static_cast<u8>(entry.getModel().mShape));
      stream.write<u8>(static_cast<u8>(entry.mType));
      stream.write<u8>(entry.mCameraIndex);
      stream.write<u8>(entry.mPriority);
      entry.getModel().mPosition >> stream;
      entry.getModel().mRotation >> stream;
      entry.getModel().mScaling >> stream;
      for (auto p : entry.mParameters)
        stream.write<u16>(p);
      if (map.mRevision >= 2200) {
        stream.write<u8>(entry.mRailID);
        stream.write<u8>(entry.mEnemyLinkID);
        for (auto p : entry.mPad)
          stream.write<u8>(p);
      }
    }
  });

  reloc.writeReloc<s32>("KMP", "CAME", [&](oishii::Writer& stream) {
    stream.write<u32>('CAME');
    stream.write<u16>(map.mCameras.size());
    // Ignored < 1920
    stream.write<u8>(map.mOpeningPanIndex);
    stream.write<u8>(map.mVideoPanIndex);
    for (auto& entry : map.mCameras) {
      stream.write<u8>(static_cast<u8>(entry.mType));
      stream.write<u8>(entry.mNext);
      stream.write<u8>(entry.mShake);
      stream.write<u8>(entry.mPathId);
      stream.write<u16>(entry.mPathSpeed);
      stream.write<u16>(entry.mFov.mSpeed);
      stream.write<u16>(entry.mView.mSpeed);
      stream.write<u8>(entry.mStartFlag);
      stream.write<u8>(entry.mMovieFlag);
      entry.mPosition >> stream;
      entry.mRotation >> stream;
      stream.write<f32>(entry.mFov.from);
      stream.write<f32>(entry.mFov.to);
      entry.mView.from >> stream;
      entry.mView.to >> stream;
      stream.write<f32>(entry.mActiveFrames);
    }
  });
  reloc.writeReloc<s32>("KMP", "JGPT", [&](oishii::Writer& stream) {
    stream.write<u32>('JGPT');
    stream.write<u16>(map.mRespawnPoints.size());
    stream.write<u16>(0); // user data
    for (auto& entry : map.mRespawnPoints) {
      entry.position >> stream;
      entry.rotation >> stream;
      writer.write<u16>(entry.id);
      writer.write<u16>(entry.range);
    }
  });
  reloc.writeReloc<s32>("KMP", "CNPT", [&](oishii::Writer& stream) {
    stream.write<u32>('CNPT');
    stream.write<u16>(map.mCannonPoints.size());
    stream.write<u16>(0); // user data
    int i = 0;
    for (auto& entry : map.mCannonPoints) {
      entry.mPosition >> stream;
      entry.mRotation >> stream;
      stream.write<u16>(i++);
      stream.write<u16>(static_cast<u16>(entry.mType));
    }
  });

  reloc.writeReloc<s32>("KMP", "MSPT", [&](oishii::Writer& stream) {
    stream.write<u32>('MSPT');
    stream.write<u16>(map.mMissionPoints.size());
    stream.write<u16>(0); // user data
    for (auto& entry : map.mMissionPoints) {
      entry.position >> stream;
      entry.rotation >> stream;
      writer.write<u16>(entry.id);
      writer.write<u16>(entry.unknown);
    }
  });

  reloc.writeReloc<s32>("KMP", "STGI", [&](oishii::Writer& stream) {
    stream.write<u32>('STGI');
    stream.write<u16>(map.mStages.size());
    stream.write<u16>(0); // user data
    for (auto& entry : map.mStages) {
      stream.write<u8>(entry.mLapCount);
      stream.write<u8>(static_cast<u8>(entry.mCorner));
      stream.write<u8>(static_cast<u8>(entry.mStartPosition));
      stream.write<u8>(entry.mFlareTobi);
      stream.write<u8>(entry.mLensFlareOptions.a);
      stream.write<u8>(entry.mLensFlareOptions.r);
      stream.write<u8>(entry.mLensFlareOptions.g);
      stream.write<u8>(entry.mLensFlareOptions.b);

      if (map.mRevision >= 2320) {
        stream.write<u8>(entry.mUnk08);
        stream.write<u8>(entry._);
        stream.write<u16>(entry.mSpeedModifier);
      } else {
        // Speed mod value will be defined by whatever comes next in the
        // archive. Nice.
      }
    }
  });
  reloc.label("KMP");
  // Write and sections
  reloc.writeChildren();

  reloc.label("KMP_END");

  // Link relocs
  reloc.resolve();
}

} // namespace librii::kmp
