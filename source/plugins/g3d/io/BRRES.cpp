#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/Dictionary.hpp>
#include <plugins/g3d/util/NameTable.hpp>

#include <set>
#include <string>

#include "Common.hpp"

#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/TextureIO.hpp>

namespace riistudio::g3d {

void WriteBone(riistudio::g3d::NameTable& names, oishii::Writer& writer,
               const size_t& bone_start, const riistudio::g3d::Bone& bone,
               const u32& bone_id);

void WriteMesh(oishii::Writer& writer, const riistudio::g3d::Polygon& mesh,
               const riistudio::g3d::Model& mdl, const size_t& mesh_start,
               riistudio::g3d::NameTable& names);
// MDL0.cpp
void writeModel(const Model& mdl, oishii::Writer& writer, RelocWriter& linker,
                NameTable& names, std::size_t brres_start);
void readModel(Model& mdl, oishii::BinaryReader& reader,
               kpi::LightIOTransaction& transaction,
               const std::string& transaction_path);

// TEX0.cpp
void writeTexture(const Texture& data, oishii::Writer& writer,
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
  Dictionary rootDict(reader);

  for (std::size_t i = 1; i < rootDict.mNodes.size(); ++i) {
    const auto& cnode = rootDict.mNodes[i];

    reader.seekSet(cnode.mDataDestination);
    Dictionary cdic(reader);

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
    } else {
      transaction.callback(kpi::IOMessageClass::Warning, "/" + cnode.mName,
                           "[WILL NOT BE SAVED] Unsupported folder: " +
                               cnode.mName);

      printf("Unsupported folder: %s\n", cnode.mName.c_str());

      if (cnode.mName == "AnmTexSrt(NW4R)") {
        for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
          const auto& sub = cdic.mNodes[j];

          reader.seekSet(sub.mDataDestination);

          auto& srt = collection.getAnim_Srts().add();
          const bool ok = librii::g3d::ReadSrtFile(srt, SliceStream(reader));

          if (!ok) {
            transaction.callback(kpi::IOMessageClass::Warning,
                                 "/" + cnode.mName,
                                 "Failed to read SRT0: " + sub.mName);
          }
        }
      }
    }
  }
}

void WriteBRRES(Collection& collection, oishii::Writer& writer) {
  writer.setEndian(std::endian::big);

  RelocWriter linker(writer);
  NameTable names;

  const auto start = writer.tell();
  linker.label("BRRES");

  writer.write<u32>('bres');                    // magic
  writer.write<u16>(0xfeff);                    // bom
  writer.write<u16>(0);                         // revision
  linker.writeReloc<u32>("BRRES", "BRRES_END"); // filesize
  writer.write<u16>(0x10);                      // data offset
  writer.write<u16>(1 + collection.getModels().size() +
                    collection.getTextures().size()); // section count

  struct RootDictionary {
    RootDictionary(Collection& collection, oishii::Writer& writer)
        : mCollection(collection), mWriter(writer), write_pos(writer.tell()) {
      if (hasModels())
        rootDict.emplace("3DModels(NW4R)");
      if (hasTextures())
        rootDict.emplace("Textures(NW4R)");
    }
    void setModels(u32 ofs) {
      if (hasModels())
        rootDict.mNodes[1].setDataDestination(ofs);
    }
    void setTextures(u32 ofs) {
      if (hasTextures())
        rootDict.mNodes[1 + hasModels()].setDataDestination(ofs);
    }
    bool hasModels() const {
      return mCollection.getModels().begin() != mCollection.getModels().end();
    }
    bool hasTextures() const {
      return mCollection.getTextures().begin() !=
             mCollection.getTextures().end();
    }
    int numFolders() const {
      return (hasModels() ? 1 : 0) + (hasTextures() ? 1 : 0);
    }
    int computeSize() const { return 8 + rootDict.computeSize(); }
    void write(NameTable& table) {
      const auto back = mWriter.tell();
      mWriter.seekSet(write_pos);

      rootDict.calcNodes();
      mWriter.write<u32>('root');

      auto tex = mCollection.getTextures();
      auto mdl = mCollection.getModels();
      const auto mnodes_size = 8 + 16 * (mdl.size() ? 1 + mdl.size() : 0);
      const auto tnodes_size = 8 + 16 * (tex.size() ? 1 + tex.size() : 0);
      mWriter.write<u32>(rootDict.computeSize() + mnodes_size + tnodes_size);
      rootDict.write(mWriter, table);

      mWriter.seekSet(back);
    }

    Collection& mCollection;
    oishii::Writer& mWriter;
    u32 write_pos;
    QDictionary rootDict;
  };

  // The root dictionary will remember its position in stream.
  // Skip it for now.
  RootDictionary root_dict(collection, writer);
  writer.skip(root_dict.computeSize());

  QDictionary models_dict, textures_dict;

  for (auto& mdl : collection.getModels())
    models_dict.emplace(mdl.getName());
  for (auto& tex : collection.getTextures())
    textures_dict.emplace(tex.getName());

  const auto subdicts_pos = writer.tell();
  if (root_dict.hasModels())
    writer.skip(models_dict.computeSize());
  if (root_dict.hasTextures())
    writer.skip(textures_dict.computeSize());

  for (int i = 0; i < collection.getModels().size(); ++i) {
    writer.alignTo(32);
    models_dict.mNodes[i + 1].setDataDestination(writer.tell());
    auto mdl_linker = linker.sublet("Models/" + std::to_string(i));
    writeModel(collection.getModels()[i], writer, mdl_linker, names, start);
  }
  for (int i = 0; i < collection.getTextures().size(); ++i) {
    writer.alignTo(32);
    textures_dict.mNodes[i + 1].setDataDestination(writer.tell());
    writeTexture(collection.getTextures()[i], writer, names);
  }
  const auto end = writer.tell();
  writer.seekSet(subdicts_pos);
  if (root_dict.hasModels()) {
    root_dict.setModels(writer.tell());
    models_dict.write(writer, names);
  }
  if (root_dict.hasTextures()) {
    root_dict.setTextures(writer.tell());
    textures_dict.write(writer, names);
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
