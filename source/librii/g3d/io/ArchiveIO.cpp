#include "ArchiveIO.hpp"

#include "CommonIO.hpp"
// XXX: Must not include DictIO.hpp when using DictWriteIO.hpp
#include <librii/g3d/io/DictWriteIO.hpp>

#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>

// Enable DictWriteIO (which is half broken: it works for reading, but not
// writing)
namespace librii::g3d {
using namespace bad;
}

namespace librii::g3d {

void BinaryArchive::read(oishii::BinaryReader& reader,
                         kpi::LightIOTransaction& transaction) {
  // Magic
  reader.read<u32>();
  // byte order
  reader.read<u16>();
  // revision
  reader.read<u16>();
  // filesize
  reader.read<u32>();
  // ofs
  reader.read<u16>();
  // section size
  reader.read<u16>();

  // 'root'
  reader.read<u32>();
  // Length of the section
  reader.read<u32>();
  librii::g3d::Dictionary rootDict(reader);

  for (std::size_t i = 1; i < rootDict.mNodes.size(); ++i) {
    const auto& cnode = rootDict.mNodes[i];

    reader.seekSet(cnode.mDataDestination);
    librii::g3d::Dictionary cdic(reader);

    // TODO
    if (cnode.mName == "3DModels(NW4R)") {
      if (cdic.mNodes.size() > 2) {
        transaction.callback(
            kpi::IOMessageClass::Error, cnode.mName,
            "This file has multiple MDL0 files within it. "
            "Only single-MDL0 BRRES files are currently supported.");
        transaction.state = kpi::TransactionState::Failure;
      }
      for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
        const auto& sub = cdic.mNodes[j];

        reader.seekSet(sub.mDataDestination);
        auto& mdl = models.emplace_back();
        bool isValid = true;
        mdl.read(reader, transaction, "/" + cnode.mName + "/" + sub.mName + "/",
                 isValid);
        (void)isValid;
      }
    } else if (cnode.mName == "Textures(NW4R)") {
      for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
        const auto& sub = cdic.mNodes[j];

        reader.seekSet(sub.mDataDestination);
        auto& tex = textures.emplace_back();
        const bool ok =
            librii::g3d::ReadTexture(tex, SliceStream(reader), sub.mName);

        if (!ok) {
          transaction.callback(kpi::IOMessageClass::Warning, "/" + cnode.mName,
                               "Failed to read texture: " + sub.mName);
        }
      }
    } else if (cnode.mName == "AnmTexSrt(NW4R)") {
      for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
        const auto& sub = cdic.mNodes[j];

        reader.seekSet(sub.mDataDestination);

        auto& srt = srts.emplace_back();
        const bool ok = librii::g3d::ReadSrtFile(srt, SliceStream(reader));

        if (!ok) {
          transaction.callback(kpi::IOMessageClass::Warning, "/" + cnode.mName,
                               "Failed to read SRT0: " + sub.mName);
        }
      }
    } else {
      transaction.callback(kpi::IOMessageClass::Warning, "/" + cnode.mName,
                           "[WILL NOT BE SAVED] Unsupported folder: " +
                               cnode.mName);

      printf("Unsupported folder: %s\n", cnode.mName.c_str());
    }
  }
}

namespace {

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

void WriteBRRES(librii::g3d::BinaryArchive& arc, oishii::Writer& writer) {
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
    header.section_count =
        1 + arc.models.size() + arc.textures.size() + arc.srts.size();
    header.write(writer, linker);
  }

  struct RootDictionary {
    RootDictionary(librii::g3d::BinaryArchive& collection,
                   oishii::Writer& writer)
        : mCollection(collection), mWriter(writer), write_pos(writer.tell()) {}

    librii::g3d::BetterNode models{.name = "3DModels(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode textures{.name = "Textures(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode srts{.name = "AnmTexSrt(NW4R)", .stream_pos = 0};

    void setModels(u32 ofs) { models.stream_pos = ofs; }
    void setTextures(u32 ofs) { textures.stream_pos = ofs; }

    bool hasModels() const { return mCollection.models.size(); }
    bool hasTextures() const { return mCollection.textures.size(); }
    bool hasSRTs() const { return mCollection.srts.size(); }

    int numFolders() const { return hasModels() + hasTextures() + hasSRTs(); }
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
          librii::g3d::CalcDictionarySize(mCollection.srts.size()));

      librii::g3d::BetterDictionary tmp;
      if (hasModels())
        tmp.nodes.push_back(models);
      if (hasTextures())
        tmp.nodes.push_back(textures);
      if (hasSRTs())
        tmp.nodes.push_back(srts);
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
  Folder srts_dict(arc.srts);

  const auto subdicts_pos = writer.tell();
  writer.skip(models_dict.computeSize());
  writer.skip(textures_dict.computeSize());
  writer.skip(srts_dict.computeSize());

  for (int i = 0; i < arc.models.size(); ++i) {
    auto& mdl = arc.models[i];

    writer.alignTo(32);

    models_dict.insert(i, mdl.name, writer.tell());

    auto mdl_linker = linker.sublet("Models/" + std::to_string(i));
    mdl.write(writer, linker, names, start, arc.textures);
  }
  for (int i = 0; i < arc.textures.size(); ++i) {
    auto& tex = arc.textures[i];

    writer.alignTo(32);

    textures_dict.insert(i, tex.name, writer.tell());

    writeTexture(tex, writer, names);
  }
  for (int i = 0; i < arc.srts.size(); ++i) {
    auto& srt = arc.srts[i];

    // SRTs are not aligned
    // writer.alignTo(32);

    srts_dict.insert(i, srt.name, writer.tell());

    librii::g3d::WriteSrtFile(writer, srt, names, start);
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
  if (arc.srts.size()) {
    root_dict.srts.stream_pos = writer.tell();
    srts_dict.write(writer, names);
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
}

} // namespace

void BinaryArchive::write(oishii::Writer& writer) {
  //
  WriteBRRES(*this, writer);
}

} // namespace librii::g3d
