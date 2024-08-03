#include "ArchiveIO.hpp"

#include "CommonIO.hpp"
#include <librii/g3d/data/Archive.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>

namespace librii::g3d {

Archive::Archive() __attribute__((weak)) = default;
Archive::Archive(const Archive&) __attribute__((weak)) = default;
Archive::Archive(Archive&&) noexcept __attribute__((weak)) = default;
Archive::~Archive() __attribute__((weak)) = default;

struct BRRESHeader {
  u32 magic;
  u16 bom;
  u16 revision;
  std::string filesize_reloc_a, filesize_reloc_b;
  u16 data_offset;
  u16 section_count;
  void write(oishii::Writer& writer, RelocWriter& linker) {
    writer.write<u32>(magic);                                   // magic
    writer.write<u16>(bom);                                     // bom
    writer.write<u16>(revision);                                // revision
    linker.writeReloc<u32>(filesize_reloc_a, filesize_reloc_b); // filesize
    writer.write<u16>(data_offset);                             // data offset
    writer.write<u16>(section_count);                           // section count
  }
};
struct BRRESHeader2 {
  u32 magic;
  u16 bom;
  u16 revision;
  u32 filesize;
  u16 data_offset;
  u16 section_count;
  static Result<BRRESHeader2> read(rsl::SafeReader& r) {
    BRRESHeader2 tmp;
    tmp.magic = TRY(r.U32());
    tmp.bom = TRY(r.U16());
    tmp.revision = TRY(r.U16());
    tmp.filesize = TRY(r.U32());
    tmp.data_offset = TRY(r.U16());
    tmp.section_count = TRY(r.U16());
    return tmp;
  }
};

Result<void> BinaryArchive::read(oishii::BinaryReader& reader,
                                 kpi::LightIOTransaction& transaction) {
  rsl::SafeReader safe(reader);
  TRY(BRRESHeader2::read(safe)); // TODO: Validate fields

  // 'root'
  TRY(safe.Magic("root"));
  // Length of the section
  TRY(safe.U32());
  auto rootDict = TRY(ReadDictionary(safe));

  for (auto& node : rootDict.nodes) {
    EXPECT(node.stream_pos);
    reader.seekSet(node.stream_pos);
    auto cdic = TRY(ReadDictionary(safe));

    // TODO
    if (node.name == "3DModels(NW4R)") {
      for (auto& sub : cdic.nodes) {
        EXPECT(sub.stream_pos);
        reader.seekSet(sub.stream_pos);
        auto& mdl = models.emplace_back();
        bool isValid = true;
        auto ok = mdl.read(reader, transaction,
                           "/" + node.name + "/" + sub.name + "/", isValid);
        if (!ok) {
          return RSL_UNEXPECTED(
              std::format("Failed to read MDL0 {}: {}", sub.name, ok.error()));
        }
        (void)isValid;
      }
    } else if (node.name == "Textures(NW4R)") {
      for (auto& sub : cdic.nodes) {
        EXPECT(sub.stream_pos);
        reader.seekSet(sub.stream_pos);
        auto& tex = textures.emplace_back();
        const bool ok =
            librii::g3d::ReadTexture(tex, SliceStream(reader), sub.name);

        if (!ok) {
          transaction.callback(kpi::IOMessageClass::Warning, "/" + node.name,
                               "Failed to read texture: " + sub.name);
        }
      }
    } else if (node.name == "AnmChr(NW4R)") {
      for (auto& sub : cdic.nodes) {
        EXPECT(sub.stream_pos);
        reader.seekSet(sub.stream_pos);

        auto warn = [&](std::string_view msg) {
          transaction.callback(kpi::IOMessageClass::Warning,
                               std::format("CHR0 {}", sub.name), msg);
        };
        auto& chr = chrs.emplace_back();
        auto ok = chr.read(reader, warn);
        if (!ok) {
          return RSL_UNEXPECTED(
              std::format("Failed to read CHR0 {}: {}", sub.name, ok.error()));
        }
      }
    } else if (node.name == "AnmClr(NW4R)") {
      for (auto& sub : cdic.nodes) {
        EXPECT(sub.stream_pos);
        reader.seekSet(sub.stream_pos);

        auto& clr = clrs.emplace_back();
        auto ok = clr.read(reader);
        if (!ok) {
          return RSL_UNEXPECTED(
              std::format("Failed to read CLR0 {}: {}", sub.name, ok.error()));
        }
      }
    } else if (node.name == "AnmTexPat(NW4R)") {
      for (auto& sub : cdic.nodes) {
        EXPECT(sub.stream_pos);
        reader.seekSet(sub.stream_pos);

        auto& pat = pats.emplace_back();
        auto ok = pat.read(reader);
        if (!ok) {
          return RSL_UNEXPECTED(
              std::format("Failed to read PAT0 {}: {}", sub.name, ok.error()));
        }
      }
    } else if (node.name == "AnmTexSrt(NW4R)") {
      for (auto& sub : cdic.nodes) {
        EXPECT(sub.stream_pos);
        reader.seekSet(sub.stream_pos);

        auto& srt = srts.emplace_back();
        auto ok = srt.read(reader);
        if (!ok) {
          return RSL_UNEXPECTED(
              std::format("Failed to read SRT0 {}: {}", sub.name, ok.error()));
        }
      }
    } else if (node.name == "AnmVis(NW4R)") {
      for (auto& sub : cdic.nodes) {
        EXPECT(sub.stream_pos);
        reader.seekSet(sub.stream_pos);

        auto& vis = viss.emplace_back();
        auto ok = vis.read(reader);
        if (!ok) {
          return RSL_UNEXPECTED(
              std::format("Failed to read VIS0 {}: {}", sub.name, ok.error()));
        }
      }
    } else {
      transaction.callback(kpi::IOMessageClass::Warning, "/" + node.name,
                           "[WILL NOT BE SAVED] Unsupported folder: " +
                               node.name);

      rsl::error("Unsupported folder: {}", node.name.c_str());
    }
  }

  return {};
}

namespace {

struct Folder {
  Folder(const auto& collection) : m_numEntries(collection.size()) {
    m_impl.nodes.resize(collection.size());
  }
  void skipSize(oishii::Writer& writer) {
    writer.skip(librii::g3d::CalcDictionarySize(m_numEntries));
  }
  u32 computeSize() { return librii::g3d::CalcDictionarySize(m_numEntries); }
  void insert(size_t i, const std::string& name, u32 stream_pos) {
    librii::g3d::BetterNode node{.name = name, .stream_pos = stream_pos};
    m_impl.nodes[i] = node;
  }
  void write(oishii::Writer& writer, NameTable& names) {
    WriteDictionary(m_impl, writer, names);
  }
  int m_numEntries = 0;
  librii::g3d::BetterDictionary m_impl;
};

inline void ApplyGlobalRelocToOishii(NameTable& table, oishii::Writer& writer,
                                     librii::g3d::NameReloc reloc) {
  table.reserve(reloc.name,                        // name
                reloc.offset_of_delta_reference,   // structPos
                writer,                            // writeStream
                reloc.offset_of_pointer_in_struct, // writePos
                reloc.non_volatile                 // nonvolatile
  );
}
struct RelocationToApply {
  RelocationToApply(NameTable& table, oishii::Writer& writer, u32 start)
      : mTable(table), mWriter(writer), mStart(start) {}
  ~RelocationToApply() { apply(); }

  operator librii::g3d::NameReloc&() { return mReloc; }

private:
  void apply() {
    mReloc.offset_of_delta_reference += mStart;
    mReloc.offset_of_pointer_in_struct += mStart;
    ApplyGlobalRelocToOishii(mTable, mWriter, mReloc);
  }

  NameTable& mTable;
  oishii::Writer& mWriter;
  u32 mStart;
  librii::g3d::NameReloc mReloc;
};

inline std::pair<u32, std::span<u8>> HandleBlock(oishii::Writer& writer,
                                                 librii::g3d::BlockData block) {
  writer.alignTo(block.start_align);
  const auto start_addr = writer.reserveNext(block.size);
  writer.seekSet(start_addr + block.size);

  std::span<u8> span(writer.getDataBlockStart() + start_addr, block.size);
  return {start_addr, span};
}

void writeTexture(const librii::g3d::TextureData& data, oishii::Writer& writer,
                  NameTable& names) {
  const auto [start, span] =
      HandleBlock(writer, librii::g3d::CalcTextureBlockData(data));

  librii::g3d::WriteTexture(span, data, -start,
                            RelocationToApply{names, writer, start});
}

Result<void> WriteBRRES(librii::g3d::BinaryArchive& arc,
                        oishii::Writer& writer) {
  writer.setEndian(std::endian::big);

  RelocWriter linker(writer);
  NameTable names;

  const auto start = writer.tell();
  linker.label("BRRES");
  {
    BRRESHeader header;
    header.magic = 'bres';
    header.bom = 0xfeff;
    header.revision = 0;
    header.filesize_reloc_a = "BRRES";
    header.filesize_reloc_b = "BRRES_END";
    header.data_offset = 0x10;
    header.section_count = 1 + arc.models.size() + arc.textures.size() +
                           arc.chrs.size() + arc.clrs.size() + arc.pats.size() +
                           arc.srts.size() + arc.viss.size();
    header.write(writer, linker);
  }

  struct RootDictionary {
    RootDictionary(librii::g3d::BinaryArchive& collection,
                   oishii::Writer& writer)
        : mCollection(collection), mWriter(writer), write_pos(writer.tell()) {}

    librii::g3d::BetterNode models{.name = "3DModels(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode textures{.name = "Textures(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode chrs{.name = "AnmChr(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode clrs{.name = "AnmClr(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode pats{.name = "AnmTexPat(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode srts{.name = "AnmTexSrt(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode viss{.name = "AnmVis(NW4R)", .stream_pos = 0};

    void setModels(u32 ofs) { models.stream_pos = ofs; }
    void setTextures(u32 ofs) { textures.stream_pos = ofs; }

    bool hasModels() const { return mCollection.models.size(); }
    bool hasTextures() const { return mCollection.textures.size(); }
    bool hasCHRs() const { return mCollection.chrs.size(); }
    bool hasCLRs() const { return mCollection.clrs.size(); }
    bool hasPATs() const { return mCollection.pats.size(); }
    bool hasSRTs() const { return mCollection.srts.size(); }
    bool hasVISs() const { return mCollection.viss.size(); }

    int numFolders() const {
      return (int)hasModels() + (int)hasTextures() + (int)hasCHRs() +
             (int)hasSRTs() + (int)hasPATs() + (int)hasCLRs() + (int)hasVISs();
    }
    int computeSize() const {
      return 8 + librii::g3d::CalcDictionarySize(numFolders());
    }
    void write(NameTable& table) {
      const auto back = mWriter.tell();
      mWriter.seekSet(write_pos);

      mWriter.write<u32>('root');

      auto& tex = mCollection.textures;
      auto& mdl = mCollection.models;
      mWriter.write<u32>(
          computeSize() + librii::g3d::CalcDictionarySize(mdl.size()) +
          librii::g3d::CalcDictionarySize(tex.size()) +
          librii::g3d::CalcDictionarySize(mCollection.chrs.size()) +
          librii::g3d::CalcDictionarySize(mCollection.clrs.size()) +
          librii::g3d::CalcDictionarySize(mCollection.pats.size()) +
          librii::g3d::CalcDictionarySize(mCollection.srts.size()) +
          librii::g3d::CalcDictionarySize(mCollection.viss.size()));

      librii::g3d::BetterDictionary tmp;
      if (hasModels())
        tmp.nodes.push_back(models);
      if (hasTextures())
        tmp.nodes.push_back(textures);
      if (hasCHRs())
        tmp.nodes.push_back(chrs);
      if (hasCLRs())
        tmp.nodes.push_back(clrs);
      if (hasPATs())
        tmp.nodes.push_back(pats);
      if (hasSRTs())
        tmp.nodes.push_back(srts);
      if (hasVISs())
        tmp.nodes.push_back(viss);
      WriteDictionary(tmp, mWriter, table);

      mWriter.seekSet(back);
    }

    librii::g3d::BinaryArchive& mCollection;
    oishii::Writer& mWriter;
    u32 write_pos;
  };

  // The root dictionary will remember its position in stream.
  // Skip it for now.
  RootDictionary root_dict(arc, writer);
  writer.skip(root_dict.computeSize());

  Folder models_dict(arc.models);
  Folder textures_dict(arc.textures);
  Folder chrs_dict(arc.chrs);
  Folder clrs_dict(arc.clrs);
  Folder pats_dict(arc.pats);
  Folder srts_dict(arc.srts);
  Folder viss_dict(arc.viss);

  const auto subdicts_pos = writer.tell();
  writer.skip(models_dict.computeSize());
  writer.skip(textures_dict.computeSize());
  writer.skip(chrs_dict.computeSize());
  writer.skip(clrs_dict.computeSize());
  writer.skip(pats_dict.computeSize());
  writer.skip(srts_dict.computeSize());
  writer.skip(viss_dict.computeSize());

  for (int i = 0; i < arc.models.size(); ++i) {
    auto& mdl = arc.models[i];

    writer.alignTo(32);

    models_dict.insert(i, mdl.name, writer.tell());

    TRY(mdl.write(writer, names, start));
  }
  for (int i = 0; i < arc.textures.size(); ++i) {
    auto& tex = arc.textures[i];

    writer.alignTo(32);

    textures_dict.insert(i, tex.name, writer.tell());

    writeTexture(tex, writer, names);
  }
  for (int i = 0; i < arc.chrs.size(); ++i) {
    auto& chr = arc.chrs[i];

    chrs_dict.insert(i, chr.name, writer.tell());

    chr.write(writer, names, start);
  }
  for (int i = 0; i < arc.clrs.size(); ++i) {
    auto& clr = arc.clrs[i];

    clrs_dict.insert(i, clr.name, writer.tell());

    clr.write(writer, names, start);
  }
  for (int i = 0; i < arc.pats.size(); ++i) {
    auto& pat = arc.pats[i];

    pats_dict.insert(i, pat.name, writer.tell());

    TRY(pat.write(writer, names, start));
  }
  for (int i = 0; i < arc.srts.size(); ++i) {
    auto& srt = arc.srts[i];

    // SRTs are not aligned
    // writer.alignTo(32);

    srts_dict.insert(i, srt.name, writer.tell());

    TRY(srt.write(writer, names, start));
  }
  for (int i = 0; i < arc.viss.size(); ++i) {
    auto& vis = arc.viss[i];

    viss_dict.insert(i, vis.name, writer.tell());

    vis.write(writer, names, start);
  }
  const auto end = writer.tell();

  writer.seekSet(subdicts_pos);
  if (arc.models.size()) {
    root_dict.setModels(writer.tell());
    models_dict.write(writer, names);
  }
  if (arc.textures.size()) {
    root_dict.setTextures(writer.tell());
    textures_dict.write(writer, names);
  }
  if (arc.chrs.size()) {
    root_dict.chrs.stream_pos = writer.tell();
    chrs_dict.write(writer, names);
  }
  if (arc.clrs.size()) {
    root_dict.clrs.stream_pos = writer.tell();
    clrs_dict.write(writer, names);
  }
  if (arc.pats.size()) {
    root_dict.pats.stream_pos = writer.tell();
    pats_dict.write(writer, names);
  }
  if (arc.srts.size()) {
    root_dict.srts.stream_pos = writer.tell();
    srts_dict.write(writer, names);
  }
  if (arc.viss.size()) {
    root_dict.viss.stream_pos = writer.tell();
    viss_dict.write(writer, names);
  }
  root_dict.write(names);
  {
    names.poolNames();
    names.resolve(end);
    writer.seekSet(end);
    for (auto p : names.mPool)
      writer.write<u8>(p);
  }

  writer.alignTo(128);
  linker.label("BRRES_END");

  linker.resolve();
  linker.printLabels();

  writer.seekSet(0);
  writer.write<u32>('bres'); // magic
  writer.write<u16>(0xfeff); // bom

  return {};
}

} // namespace

Result<void> BinaryArchive::write(oishii::Writer& writer) {
  //
  return WriteBRRES(*this, writer);
}

//
// Intermediate
//
Result<Archive> Archive::from(const BinaryArchive& archive,
                              kpi::LightIOTransaction& transaction) {
  Archive tmp;
  for (auto& mdl : archive.models) {
    tmp.models.emplace_back(
        TRY(Model::from(mdl, transaction, "MDL0 " + mdl.name)));
  }
  tmp.textures = archive.textures;
  tmp.chrs = archive.chrs;
  for (auto& clr : archive.clrs) {
    tmp.clrs.push_back(TRY(ClrAnim::from(clr)));
  }
  for (auto& pat : archive.pats) {
    tmp.pats.push_back(TRY(PatAnim::from(pat)));
  }

  for (auto& srt : archive.srts) {
    auto srt_warn = [&](std::string_view msg) {
      transaction.callback(kpi::IOMessageClass::Warning,
                           std::format("SRT0 {}", srt.name), msg);
    };
    SrtAnim json = TRY(SrtAnim::read(srt, srt_warn));
    auto b2 = SrtAnim::write(json);
    if (srt != b2) {
      transaction.callback(kpi::IOMessageClass::Warning,
                           std::format("SRT0 {}", srt.name),
                           "SrtAnim re-encode will not be byte-matching.");
    }
    tmp.srts.emplace_back(json);
  }
  tmp.viss = archive.viss;
  // TestJson(tmp);
  return tmp;
}
Result<BinaryArchive> Archive::binary() const {
  BinaryArchive tmp;
  for (auto& mdl : models) {
    tmp.models.emplace_back(TRY(mdl.binary()));
  }
  tmp.textures = textures;
  tmp.chrs = chrs;
  for (auto& clr : clrs) {
    tmp.clrs.push_back(clr.to());
  }
  for (auto& pat : pats) {
    tmp.pats.push_back(pat.to());
  }
  for (auto& srt : srts) {
    // TODO: Actually bind to the proper model?
    tmp.srts.emplace_back(
        srt.write(srt, models.empty() ? nullptr : &models[0]));
  }
  tmp.viss = viss;
  return tmp;
}
Result<void> Archive::write(oishii::Writer& writer) const {
  return TRY(binary()).write(writer);
}
Result<void> Archive::write(std::string_view path) const {
  oishii::Writer writer(std::endian::big);
  TRY(write(writer));
  writer.saveToDisk(path);
  return {};
}
Result<std::vector<u8>> Archive::write() const {
  oishii::Writer writer(std::endian::big);
  TRY(write(writer));
  return writer.mBuf;
}

Result<Archive> Archive::fromFile(std::string path,
                                  kpi::LightIOTransaction& transaction) {
  auto reader = oishii::BinaryReader::FromFilePath(path, std::endian::big);
  EXPECT(reader && "Failed to read file");
  return read(*reader, transaction);
}
Result<Archive> Archive::fromFile(std::string path) {
  kpi::LightIOTransaction trans;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    rsl::error(msg);
  };
  return fromFile(path, trans);
}
Result<Archive> Archive::read(oishii::BinaryReader& reader,
                              kpi::LightIOTransaction& transaction) {
  BinaryArchive bin;
  TRY(bin.read(reader, transaction));
  return from(bin, transaction);
}
Result<Archive> Archive::fromMemory(std::span<const u8> buf, std::string path,
                                    kpi::LightIOTransaction& trans) {
  oishii::BinaryReader reader(buf, path, std::endian::big);
  return read(reader, trans);
}
Result<Archive> Archive::fromMemory(std::span<const u8> buf, std::string path) {
  kpi::LightIOTransaction trans;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    rsl::error(msg);
  };
  return fromMemory(buf, path, trans);
}

} // namespace librii::g3d
