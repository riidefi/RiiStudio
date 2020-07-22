#include "KMP.hpp"
#include <core/util/glm_io.hpp>
#include <functional>
#include <llvm/ADT/StringMap.h>
#include <numeric>
#include <oishii/writer/binary_writer.hxx>
#include <sstream>
#include <tuple>
#include <vector>

namespace riistudio::mk {

struct IOContext {
  std::string path;
  kpi::IOTransaction& transaction;

  auto sublet(const std::string& dir) {
    return IOContext(path + "/" + dir, transaction);
  }

  void callback(kpi::IOMessageClass mclass, const std::string_view message) {
    transaction.callback(mclass, path, message);
  }

  void inform(const std::string_view message) {
    callback(kpi::IOMessageClass::Information, message);
  }
  void warn(const std::string_view message) {
    callback(kpi::IOMessageClass::Warning, message);
  }
  void error(const std::string_view message) {
    callback(kpi::IOMessageClass::Error, message);
  }
  void request(bool cond, const std::string_view message) {
    if (!cond)
      warn(message);
  }
  void require(bool cond, const std::string_view message) {
    if (!cond)
      error(message);
  }

  IOContext(std::string&& p, kpi::IOTransaction& t)
      : path(std::move(p)), transaction(t) {}
  IOContext(kpi::IOTransaction& t) : transaction(t) {}
};

void KMP::read(kpi::IOTransaction& transaction) const {
  CourseMap& map = static_cast<CourseMap&>(transaction.node);
  oishii::BinaryReader reader(std::move(transaction.data));
  IOContext ctx("kmp", transaction);

  u16 num_sec = 0;
  u16 header_size = 0;

  {
    auto g = ctx.sublet("header");

    g.require(reader.read<u32>() == 'RKMD', "Invalid file magic");
    g.require(reader.read<u32>() == reader.endpos(), "Invalid file size");

    num_sec = reader.read<u16>();
    g.request(num_sec == 15, "Unusual section count");

    header_size = reader.read<u16>();
    g.require(header_size == 0x10 + num_sec * 4, "Spoopy header length");

    map.setRevision(reader.read<u32>());
    g.require(map.getRevision() == 2520, "Unusual revision");
  }

  assert(num_sec < 32);
  u32 sections_handled = 0;

  const auto search = [&](u32 key, bool standard_fields =
                                       true) -> std::tuple<bool, u16, u16> {
    for (u32 i = 0; i < num_sec; ++i) {
      reader.seekSet(header_size +
                     reader.getAt<u32>(header_size + (i - num_sec) * 4));
      const auto magic = reader.read<u32>();
      if (magic == key) {
        sections_handled |= (1 << i);
        if (!standard_fields)
          return {true, 1, 0};
        const auto num_entry = reader.read<u16>();
        const auto user_data = reader.read<u16>();
        return {true, num_entry, user_data};
      }
    }
    return {false, 0, 0};
  };

  if (auto [found, num_entry, user_data] =
          search('KTPT', map.getRevision() > 1830);
      found) {
    auto sec = map.getStartPoints();
    sec.resize(num_entry);
    for (auto& entry : sec) {
      entry.position << reader;
      entry.rotation << reader;
      entry.player_index = reader.read<s16>();
      entry._ = reader.read<u16>();
    }
  }

  const auto read_path_section = [&](u32 path_key, u32 point_key, auto sec,
                                     u32 point_stride, auto read_point) {
    auto [pt_found, pt_num_entry, pt_user_data] = search(point_key);
    if (!pt_found)
      return;
    const auto pt_ofs = reader.tell();

    auto [ph_found, ph_num_entry, ph_user_data] = search(path_key);
    if (!ph_found)
      return;

    sec.resize(ph_num_entry);
    for (auto& entry : sec) {
      const auto [start, size] = reader.readX<u8, 2>();

      for (auto& e : reader.readX<u8, 6>())
        if (e != 0xFF)
          entry.mPredecessors.push_back(e);
      for (auto& e : reader.readX<u8, 6>())
        if (e != 0xFF)
          entry.mSuccessors.push_back(e);

      entry.misc = reader.readX<u8, 2>();

      {
        oishii::Jump<oishii::Whence::Set> g(reader,
                                            pt_ofs + point_stride * start);
        entry.mPoints.resize(size);
        for (auto& pt : entry.mPoints)
          read_point(pt, reader);
      }
    }
  };

  const auto read_enpt = [](EnemyPoint& pt, oishii::BinaryReader& reader) {
    pt.position << reader;
    pt.deviation = reader.read<f32>();
    pt.param = reader.readX<u8, 4>();
  };
  read_path_section('ENPH', 'ENPT', map.getEnemyPaths(), 0x14, read_enpt);

  const auto read_itpt = [](ItemPoint& pt, oishii::BinaryReader& reader) {
    pt.position << reader;
    pt.deviation = reader.read<f32>();
    pt.param = reader.readX<u8, 4>();
  };
  read_path_section('ITPH', 'ITPT', map.getItemPaths(), 0x14, read_itpt);

  const auto read_ckpt = [&](CheckPoint& pt, oishii::BinaryReader& reader) {
    glm::vec2 scratch;
    scratch << reader;
    pt.setLeft(scratch);
    scratch << reader;
    pt.setRight(scratch);
    pt.setRespawnIndex(reader.read<u8>());
    pt.setLapCheck(reader.read<u8>());
    // TODO: We assume the intrusive linked-list data is valid
    reader.skip(2);
  };
  read_path_section('CKPH', 'CKPT', map.getCheckPaths(), 0x14, read_ckpt);

  if (auto [found, num_entry, user_data] = search('GOBJ'); found) {
    auto sec = map.getGeoObjs();
    sec.resize(num_entry);
    for (auto& entry : sec) {
      entry.id = reader.read<u16>();
      entry._ = reader.read<u16>();
      entry.position << reader;
      entry.rotation << reader;
      entry.scale << reader;
      entry.pathId = reader.read<u16>();
      entry.settings = reader.readX<u16, 8>();
      entry.flags = reader.read<u16>();
    }
  }

  if (auto [found, num_entry, user_data] = search('POTI'); found) {
    const auto total_points_expected = user_data;
    u32 total_points_real = 0;

    auto sec = map.getPaths();
    sec.resize(num_entry);
    for (auto& entry : sec) {
      const auto entry_size = reader.read<u16>();
      entry.resize(entry_size);
      total_points_real += entry_size;
      entry.setInterpolation(static_cast<Interpolation>(reader.read<u8>()));
      entry.setLoopPolicy(static_cast<LoopPolicy>(reader.read<u8>()));
      for (auto& sub : entry) {
        sub.position << reader;
        sub.params = reader.readX<u16, 2>();
      }
    }

    ctx.sublet("POTI").require(total_points_expected == total_points_real,
                               "Expected " +
                                   std::to_string(total_points_expected) +
                                   " POTI points. Instead saw " +
                                   std::to_string(total_points_real) + ".");
  }

  if (auto [found, num_entry, user_data] = search('AREA'); found) {
    auto area_ctx = ctx.sublet("Areas");
    auto sec = map.getAreas();
    sec.resize(num_entry);

    int i = 0;
    for (auto& entry : sec) {
      auto entry_ctx = area_ctx.sublet("#" + std::to_string(i++));

      auto raw_area_shp = reader.read<u8>();
      if (raw_area_shp > 1) {
        entry_ctx.error("Invalid area shape: " + std::to_string(raw_area_shp) +
                        ". Expected range: [0, 1]. Defaulting to 0 (Box).");
        raw_area_shp = 0;
      }

      auto raw_area_type = reader.read<u8>();
      if (raw_area_type > 10) {
        entry_ctx.error(
            "Invalid area type: " + std::to_string(raw_area_type) +
            ". Expected range: [0, 10]. This will be ignored  by the game.");
      }

      entry.getModel().mShape = static_cast<AreaShape>(raw_area_shp);
      entry.mType = static_cast<AreaType>(raw_area_type);
      entry.mCameraIndex = reader.read<u8>();
      entry.mPriority = reader.read<u8>();
      glm::vec3 scratch;
      scratch << reader;
      entry.getModel().setPosition(scratch);
      scratch << reader;
      entry.getModel().setRotation(scratch);
      scratch << reader;
      entry.getModel().setScaling(scratch);

      entry.mParameters = reader.readX<u16, 2>();
      if (map.getRevision() >= 2200) {
        entry.mRailID = reader.read<u8>();
        entry.mEnemyLinkID = reader.read<u8>();
        entry.mPad = reader.readX<u8, 2>();
      } else {
        entry.mRailID = 0xFF;
        entry.mEnemyLinkID = 0xFF;
        entry.mPad = {};
      }
    }
  }

  if (auto [found, num_entry, user_data] = search('CAME'); found) {
    map.setOpeningPanIndex(user_data >> 8);
    map.setVideoPanIndex(user_data & 0xff);
    auto sec = map.getCameras();
    sec.resize(num_entry);
    for (auto& entry : sec) {
      entry.mType = static_cast<CameraType>(reader.read<u8>());
      entry.mNext = reader.read<u8>();
      entry.mShake = reader.read<u8>();
      entry.mPathId = reader.read<u8>();
      entry.mPathSpeed = reader.read<u16>();
      entry.mFov.mSpeed = reader.read<u16>();
      entry.mView.mSpeed = reader.read<u16>();
      entry.mStartFlag = reader.read<u8>();
      entry.mMovieFlag = reader.read<u8>();
      entry.mPosition << reader;
      entry.mRotation << reader;
      entry.mFov.from = reader.read<f32>();
      entry.mFov.to = reader.read<f32>();
      entry.mView.from << reader;
      entry.mView.to << reader;
      entry.mActiveFrames = reader.read<f32>();
    }
  }

  if (auto [found, num_entry, user_data] = search('JGPT'); found) {
    auto sec = map.getRespawnPoints();
    sec.resize(num_entry);
    for (auto& entry : sec) {
      entry.position << reader;
      entry.rotation << reader;
      entry.id = reader.read<u16>();
      entry.range = reader.read<u16>();
    }
  }

  if (auto [found, num_entry, user_data] = search('CNPT'); found) {
    auto ctx_cnpt = ctx.sublet("Cannons");

    auto sec = map.getCannonPoints();
    sec.resize(num_entry);
    int i = 0;
    for (auto& entry : sec) {
      auto ctx_entry = ctx_cnpt.sublet(std::to_string(i));
      entry.mPosition << reader;
      entry.mRotation << reader;
      ctx_entry.require(reader.read<u16>() == i, "Invalid cannon ID");
      entry.mType = static_cast<CannonType>(reader.read<u16>());
      ++i;
    }
  }

  if (auto [found, num_entry, user_data] = search('MSPT'); found) {
    auto sec = map.getMissionPoints();
    sec.resize(num_entry);
    for (auto& entry : sec) {
      entry.position << reader;
      entry.rotation << reader;
      entry.id = reader.read<u16>();
      entry.unknown = reader.read<u16>();
    }
  }

  if (auto [found, num_entry, user_data] = search('STGI'); found) {
    auto sec = map.getStages();
    sec.resize(num_entry);
    for (auto& entry : sec) {
      entry.mLapCount = reader.read<u8>();
      entry.mCorner = static_cast<Corner>(reader.read<u8>());
      entry.mStartPosition = static_cast<StartPosition>(reader.read<u8>());
      entry.mFlareTobi = reader.read<u8>();
      entry.mLensFlareOptions.a = reader.read<u8>();
      entry.mLensFlareOptions.r = reader.read<u8>();
      entry.mLensFlareOptions.g = reader.read<u8>();
      entry.mLensFlareOptions.b = reader.read<u8>();

      if (map.getRevision() >= 2320) {
        entry.mUnk08 = reader.read<u8>();
        entry._ = reader.read<u8>();
        entry.mSpeedModifier = reader.read<u16>();
      } else {
        entry.mUnk08 = 0;
        entry._ = 0;
        entry.mSpeedModifier = 0;
      }
    }
  }
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

  llvm::SmallVector<Reloc, 64> mRelocs;
  llvm::SmallVector<
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

void KMP::write(kpi::INode& node, oishii::Writer& writer) const {
  const CourseMap& map = *dynamic_cast<CourseMap*>(&node);
  writer.setEndian(true);
  RelocWriter reloc(writer);
  reloc.label("KMP_BEGIN");

  writer.write<u32>('RKMD');
  reloc.writeReloc<u32>("KMP_BEGIN", "KMP_END");
  writer.write<u16>(15);                      // 15 sections
  writer.write<u16>(15 * sizeof(u32) + 0x10); // header size
  writer.write<u32>(map.getRevision());

  reloc.writeReloc<s32>("KMP", "KTPT", [&](oishii::Writer& stream) {
    auto sec = map.getStartPoints();
    stream.write<u32>('KTPT');
    if (map.getRevision() > 1830) {
      stream.write<u16>(sec.size());
      stream.write<u16>(0); // user data
    }
    for (auto& point : sec) {
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
          [](auto total, auto& path) { return total + path.mPoints.size(); }));
      stream.write<u16>(0); // user data

      std::size_t i = 0;
      for (auto& path : paths) {
        const auto p_start = i;
        for (auto& point : path.mPoints) {
          if constexpr (std::is_same_v<T, bool>) {
            write_point(point, stream, i, p_start,
                        p_start + path.mPoints.size());
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
      int i = 0;
      u8 ph_start_index = 0;
      for (auto& path : paths) {
        stream.write<u8>(ph_start_index);
        ph_start_index += path.mPoints.size();
        stream.write<u8>(path.mPoints.size());
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
        ++i;
      }
    };

    reloc.writeReloc<s32>("KMP", stringifyId(pt_key),
                          std::bind(point_impl, std::placeholders::_1, pt_key,
                                    paths, write_point));
    reloc.writeReloc<s32>(
        "KMP", stringifyId(ph_key),
        std::bind(path_impl, std::placeholders::_1, ph_key, paths));
  };

  const auto write_enpt = [](const EnemyPoint& point, oishii::Writer& writer) {
    assert(&writer);
    point.position >> writer;
    writer.write<f32>(point.deviation);
    for (auto p : point.param)
      writer.write<u8>(p);
  };
  write_path('ENPH', 'ENPT', map.getEnemyPaths(), write_enpt);

  const auto write_itpt = [](const ItemPoint& point, oishii::Writer& writer) {
    point.position >> writer;
    writer.write<f32>(point.deviation);
    for (auto p : point.param)
      writer.write<u8>(p);
  };
  write_path('ITPH', 'ITPT', map.getItemPaths(), write_itpt);

  const auto write_ckpt = [](const CheckPoint& point, oishii::Writer& writer,
                             std::size_t seq, std::size_t first,
                             std::size_t last) {
    point.getLeft() >> writer;
    point.getRight() >> writer;
    writer.write<u8>(point.getRespawnIndex());
    writer.write<u8>(point.getLapCheck());
    // Last, Next
    writer.write<u8>(seq <= first ? 0xFF : seq - 1);
    writer.write<u8>(seq + 1 == last ? 0xFF : seq + 1);
  };
  write_path('CKPH', 'CKPT', map.getCheckPaths(), write_ckpt, true);

  reloc.writeReloc<s32>("KMP", "GOBJ", [&](oishii::Writer& stream) {
    auto sec = map.getGeoObjs();
    stream.write<u32>('GOBJ');
    stream.write<u16>(sec.size());
    stream.write<u16>(0); // user data
    for (auto& entry : sec) {
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
    stream.write<u16>(map.getPaths().size());
    u32 total_count = 0;
    for (auto& entry : map.getPaths())
      total_count += std::distance(entry.begin(), entry.end());
    stream.write<u16>(total_count);
    for (auto& entry : map.getPaths()) {
      stream.write<u16>(std::distance(entry.begin(), entry.end()));
      stream.write<u8>(static_cast<u8>(entry.getInterpolation()));
      stream.write<u8>(static_cast<u8>(entry.getLoopPolicy()));
      for (auto& sub : entry) {
        sub.position >> stream;
        for (auto p : sub.params)
          stream.write<u16>(p);
      }
    }
  });

  reloc.writeReloc<s32>("KMP", "AREA", [&](oishii::Writer& stream) {
    stream.write<u32>('AREA');
    stream.write<u16>(map.getAreas().size());
    stream.write<u16>(0); // user data
    for (auto& entry : map.getAreas()) {
      stream.write<u8>(static_cast<u8>(entry.getModel().mShape));
      stream.write<u8>(static_cast<u8>(entry.mType));
      stream.write<u8>(entry.mCameraIndex);
      stream.write<u8>(entry.mPriority);
      entry.getModel().mPosition >> stream;
      entry.getModel().mRotation >> stream;
      entry.getModel().mScaling >> stream;
      for (auto p : entry.mParameters)
        stream.write<u16>(p);
      if (map.getRevision() >= 2200) {
        stream.write<u8>(entry.mRailID);
        stream.write<u8>(entry.mEnemyLinkID);
        for (auto p : entry.mPad)
          stream.write<u8>(p);
      }
    }
  });

  reloc.writeReloc<s32>("KMP", "CAME", [&](oishii::Writer& stream) {
    stream.write<u32>('CAME');
    stream.write<u16>(map.getCameras().size());
    // Ignored < 1920
    stream.write<u8>(map.getOpeningPanIndex());
    stream.write<u8>(map.getVideoPanIndex());
    for (auto& entry : map.getCameras()) {
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
    auto sec = map.getRespawnPoints();
    stream.write<u32>('JGPT');
    stream.write<u16>(sec.size());
    stream.write<u16>(0); // user data
    for (auto& entry : sec) {
      entry.position >> stream;
      entry.rotation >> stream;
      writer.write<u16>(entry.id);
      writer.write<u16>(entry.range);
    }
  });
  reloc.writeReloc<s32>("KMP", "CNPT", [&](oishii::Writer& stream) {
    auto sec = map.getCannonPoints();
    stream.write<u32>('CNPT');
    stream.write<u16>(sec.size());
    stream.write<u16>(0); // user data
    int i = 0;
    for (auto& entry : sec) {
      entry.mPosition >> stream;
      entry.mRotation >> stream;
      stream.write<u16>(i++);
      stream.write<u16>(static_cast<u16>(entry.mType));
    }
  });

  reloc.writeReloc<s32>("KMP", "MSPT", [&](oishii::Writer& stream) {
    auto sec = map.getMissionPoints();
    stream.write<u32>('MSPT');
    stream.write<u16>(sec.size());
    stream.write<u16>(0); // user data
    for (auto& entry : sec) {
      entry.position >> stream;
      entry.rotation >> stream;
      writer.write<u16>(entry.id);
      writer.write<u16>(entry.unknown);
    }
  });

  reloc.writeReloc<s32>("KMP", "STGI", [&](oishii::Writer& stream) {
    auto sec = map.getStages();
    stream.write<u32>('STGI');
    stream.write<u16>(sec.size());
    stream.write<u16>(0); // user data
    for (auto& entry : sec) {
      stream.write<u8>(entry.mLapCount);
      stream.write<u8>(static_cast<u8>(entry.mCorner));
      stream.write<u8>(static_cast<u8>(entry.mStartPosition));
      stream.write<u8>(entry.mFlareTobi);
      stream.write<u8>(entry.mLensFlareOptions.a);
      stream.write<u8>(entry.mLensFlareOptions.r);
      stream.write<u8>(entry.mLensFlareOptions.g);
      stream.write<u8>(entry.mLensFlareOptions.b);

      if (map.getRevision() >= 2320) {
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

} // namespace riistudio::mk
