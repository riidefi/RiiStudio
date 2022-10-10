#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <core/kpi/Plugins.hpp>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

#include <librii/g3d/io/DictWriteIO.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

#include <set>
#include <string>

#include "Common.hpp"

#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>

namespace librii::g3d {
using namespace bad;
}

namespace riistudio::g3d {

// MDL0.cpp
void writeModel(const Model& mdl, oishii::Writer& writer, RelocWriter& linker,
                NameTable& names, std::size_t brres_start);
void readModel(Model& mdl, oishii::BinaryReader& reader,
               kpi::LightIOTransaction& transaction,
               const std::string& transaction_path);

// TEX0.cpp
void writeTexture(const librii::g3d::TextureData& data, oishii::Writer& writer,
                  NameTable& names);

void ReadBRRES(Collection& collection, oishii::BinaryReader& reader,
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
        auto& mdl = collection.getModels().add();
        readModel(mdl, reader, transaction,
                  "/" + cnode.mName + "/" + sub.mName + "/");
      }
    } else if (cnode.mName == "Textures(NW4R)") {
      for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
        const auto& sub = cdic.mNodes[j];

        reader.seekSet(sub.mDataDestination);
        auto& tex = collection.getTextures().add();
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

        auto& srt = collection.getAnim_Srts().add();
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

void WriteBRRES(Collection& collection, oishii::Writer& writer) {
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
    header.section_count = 1 + collection.getModels().size() +
                           collection.getTextures().size() +
                           collection.getAnim_Srts().size();
    header.write(writer, linker);
  }

  struct RootDictionary {
    RootDictionary(Collection& collection, oishii::Writer& writer)
        : mCollection(collection), mWriter(writer), write_pos(writer.tell()) {}

    librii::g3d::BetterNode models{.name = "3DModels(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode textures{.name = "Textures(NW4R)", .stream_pos = 0};
    librii::g3d::BetterNode srts{.name = "AnmTexSrt(NW4R)", .stream_pos = 0};

    void setModels(u32 ofs) { models.stream_pos = ofs; }
    void setTextures(u32 ofs) { textures.stream_pos = ofs; }

    bool hasModels() const { return mCollection.getModels().size(); }
    bool hasTextures() const { return mCollection.getTextures().size(); }
    bool hasSRTs() const { return mCollection.getAnim_Srts().size(); }

    int numFolders() const { return hasModels() + hasTextures() + hasSRTs(); }
    int computeSize() const {
      return 8 + librii::g3d::CalcDictionarySize(numFolders());
    }
    void write(NameTable& table) {
      const auto back = mWriter.tell();
      mWriter.seekSet(write_pos);

      mWriter.write<u32>('root');

      auto tex = mCollection.getTextures();
      auto mdl = mCollection.getModels();
      mWriter.write<u32>(
          computeSize() + librii::g3d::CalcDictionarySize(mdl.size()) +
          librii::g3d::CalcDictionarySize(tex.size()) +
          librii::g3d::CalcDictionarySize(mCollection.getAnim_Srts().size()));

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

    Collection& mCollection;
    oishii::Writer& mWriter;
    u32 write_pos;
  };

  // The root dictionary will remember its position in stream.
  // Skip it for now.
  RootDictionary root_dict(collection, writer);
  writer.skip(root_dict.computeSize());

  Folder models_dict(collection.getModels());
  Folder textures_dict(collection.getTextures());
  Folder srts_dict(collection.getAnim_Srts());

  const auto subdicts_pos = writer.tell();
  writer.skip(models_dict.computeSize());
  writer.skip(textures_dict.computeSize());
  writer.skip(srts_dict.computeSize());

  for (int i = 0; i < collection.getModels().size(); ++i) {
    auto& mdl = collection.getModels()[i];

    writer.alignTo(32);

    models_dict.insert(i, mdl.getName(), writer.tell());

    auto mdl_linker = linker.sublet("Models/" + std::to_string(i));
    writeModel(mdl, writer, mdl_linker, names, start);
  }
  for (int i = 0; i < collection.getTextures().size(); ++i) {
    auto& tex = collection.getTextures()[i];

    writer.alignTo(32);

    textures_dict.insert(i, tex.getName(), writer.tell());

    writeTexture(tex, writer, names);
  }
  for (int i = 0; i < collection.getAnim_Srts().size(); ++i) {
    auto& srt = collection.getAnim_Srts()[i];

    // SRTs are not aligned
    // writer.alignTo(32);

    srts_dict.insert(i, srt.getName(), writer.tell());

    librii::g3d::WriteSrtFile(writer, srt, names, start);
  }
  const auto end = writer.tell();

  writer.seekSet(subdicts_pos);
  if (collection.getModels().size()) {
    root_dict.setModels(writer.tell());
    models_dict.write(writer, names);
  }
  if (collection.getTextures().size()) {
    root_dict.setTextures(writer.tell());
    textures_dict.write(writer, names);
  }
  if (collection.getAnim_Srts().size()) {
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

  writer.alignTo(64);
  linker.label("BRRES_END");

  linker.resolve();
  linker.printLabels();

  writer.seekSet(0);
  writer.write<u32>('bres'); // magic
  writer.write<u16>(0xfeff); // bom
}

} // namespace riistudio::g3d
