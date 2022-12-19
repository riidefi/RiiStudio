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
#include <librii/g3d/io/ArchiveIO.hpp>

namespace librii::g3d {
using namespace bad;
}

namespace riistudio::g3d {

// MDL0.cpp
librii::g3d::BinaryModel toBinaryModel(const Model& mdl);
void processModel(librii::g3d::BinaryModel& binary_model,
                  kpi::LightIOTransaction& transaction,
                  const std::string& transaction_path,
                  riistudio::g3d::Model& mdl);

// TEX0.cpp
void writeTexture(const librii::g3d::TextureData& data, oishii::Writer& writer,
                  NameTable& names);

void ReadBRRES(Collection& collection, oishii::BinaryReader& reader,
               kpi::LightIOTransaction& transaction) {
  librii::g3d::BinaryArchive archive;
  archive.read(reader, transaction);
  collection.path = reader.getFile();
  for (auto& mdl : archive.models) {
    auto& editor_mdl = collection.getModels().add();
    processModel(mdl, transaction, "MDL0 " + mdl.name, editor_mdl);
  }
  for (auto& tex : archive.textures) {
    static_cast<librii::g3d::TextureData&>(collection.getTextures().add()) =
        tex;
  }
  for (auto& srt : archive.srts) {
    static_cast<librii::g3d::SrtAnimationArchive&>(
        collection.getAnim_Srts().add()) = srt;
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

  auto texRange = collection.getTextures();
  std::vector<librii::g3d::TextureData> textures(texRange.begin(),
                                                 texRange.end());
  for (int i = 0; i < collection.getModels().size(); ++i) {
    auto& mdl = collection.getModels()[i];

    writer.alignTo(32);

    models_dict.insert(i, mdl.getName(), writer.tell());

    auto mdl_linker = linker.sublet("Models/" + std::to_string(i));
    auto bin = toBinaryModel(mdl);
    bin.write(writer, linker, names, start, textures);
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

  writer.alignTo(128);
  linker.label("BRRES_END");

  linker.resolve();
  linker.printLabels();

  writer.seekSet(0);
  writer.write<u32>('bres'); // magic
  writer.write<u16>(0xfeff); // bom
}

} // namespace riistudio::g3d
