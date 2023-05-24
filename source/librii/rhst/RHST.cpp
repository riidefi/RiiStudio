#include "RHST.hpp"
#include <oishii/reader/binary_reader.hxx>
#include <rsl/SafeReader.hpp>
#include <rsl/TaggedUnion.hpp>
#include <vendor/magic_enum/magic_enum.hpp>
#include <vendor/nlohmann/json.hpp>

IMPORT_STD;

namespace librii::rhst {

enum class RHSTToken {
  RHST_DATA_NULL = 0,

  RHST_DATA_DICT = 1,
  RHST_DATA_ARRAY = 2,
  RHST_DATA_ARRAY_DYNAMIC = 3,

  RHST_DATA_END_DICT = 4,
  RHST_DATA_END_ARRAY = 5,
  RHST_DATA_END_ARRAY_DYNAMIC = 6,

  RHST_DATA_STRING = 7,
  RHST_DATA_S32 = 8,
  RHST_DATA_F32 = 9
};

class RHSTReader {
public:
  RHSTReader(std::span<const u8> file_data)
      : mReader(file_data, "Unknown Path", std::endian::little) {
    mReader.skip(8);
  }
  ~RHSTReader() = default;

  //! A null token. Signifies termination of the file.
  struct NullToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_NULL> {};

  //! A string token. An inline string will directly succeed it.
  struct StringToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_STRING> {
    std::string_view data{};
  };

  //! A 32-bit signed integer token.
  struct S32Token : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_S32> {
    s32 data = -1;
  };

  //! A 32-bit floating-point token.
  struct F32Token : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_F32> {
    f32 data = 0.0f;
  };

  //! Signifies that a dictionary will follow.
  struct DictBeginToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_DICT> {
    std::string_view name;
    u32 size = 0;
  };

  //! Signifies that a dictionary is over. Redundancy
  struct DictEndToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_END_DICT> {};

  //! Signifies that an array will follow.
  struct ArrayBeginToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_ARRAY> {
    u32 size = 0;
  };

  //! Signifies that an array is over. Redundancy
  struct ArrayEndToken
      : public rsl::kawaiiTag<RHSTToken, RHSTToken::RHST_DATA_END_ARRAY> {};

  //! Any token	of the many supported.
  struct Token : public rsl::kawaiiUnion<RHSTToken,
                                         // All token types
                                         NullToken, StringToken, S32Token,
                                         F32Token, DictBeginToken, DictEndToken,
                                         ArrayBeginToken, ArrayEndToken> {};

  [[nodiscard]] Result<void> readTok() {
    mLastToken = TRY(_readTok());
    return {};
  }
  Token get() const { return mLastToken; }

  template <typename T> Result<const T*> expect() {
    TRY(readTok());
    return rsl::dyn_cast<T>(mLastToken);
  }

  Result<bool> expectSimple(RHSTToken type) {
    TRY(readTok());
    return mLastToken.getType() == type;
  }

  Result<bool> expectString() {
    return expectSimple(RHSTToken::RHST_DATA_STRING);
  }
  Result<bool> expectInt() { return expectSimple(RHSTToken::RHST_DATA_S32); }
  Result<bool> expectFloat() { return expectSimple(RHSTToken::RHST_DATA_F32); }

private:
  Result<std::string_view> _readString() {
    const u32 string_len = TRY(mReader.tryRead<u32>());
    const u32 start_start_pos = mReader.tell();
    mReader.skip(roundUp(string_len, 4));

    const char* string_begin =
        reinterpret_cast<const char*>(mReader.getStreamStart()) +
        start_start_pos;
    std::string_view data{string_begin, string_len};

    return data;
  }

  Result<Token> _readTok() {
    const RHSTToken type =
        TRY(rsl::enum_cast<RHSTToken>(TRY(mReader.tryRead<s32>())));

    switch (type) {
    case RHSTToken::RHST_DATA_STRING: {
      std::string_view data = TRY(_readString());

      return Token{StringToken{.data = data}};
    }
    case RHSTToken::RHST_DATA_S32: {
      const s32 data = TRY(mReader.tryRead<s32>());

      return Token{S32Token{.data = data}};
    }
    case RHSTToken::RHST_DATA_F32: {
      const f32 data = TRY(mReader.tryRead<f32>());

      return Token{F32Token{.data = data}};
    }
    case RHSTToken::RHST_DATA_DICT: {
      const u32 nchildren = TRY(mReader.tryRead<u32>());
      std::string_view name = TRY(_readString());

      return Token{DictBeginToken{.name = name, .size = nchildren}};
    }
    case RHSTToken::RHST_DATA_END_DICT: {
      return Token{DictEndToken{}};
    }
    case RHSTToken::RHST_DATA_ARRAY: {
      const u32 nchildren = TRY(mReader.tryRead<u32>());
      [[maybe_unused]] const u32 child_type = TRY(mReader.tryRead<u32>());

      return Token{ArrayBeginToken{.size = nchildren}};
    }
    case RHSTToken::RHST_DATA_END_ARRAY: {
      return Token{ArrayEndToken{}};
    }
    default:
    case RHSTToken::RHST_DATA_NULL: {
      return Token{NullToken{}};
    }
    }
  }

private:
  oishii::BinaryReader mReader;
  Token mLastToken{NullToken{}};
};

class SceneTreeReader {
public:
  SceneTreeReader(RHSTReader& reader) : mReader(reader) {}

  template <typename T> Result<void> arrayIter(T functor) {
    auto* array_begin = TRY(mReader.expect<RHSTReader::ArrayBeginToken>());
    if (array_begin == nullptr)
      return std::unexpected("Expected an array");

    auto begin_data = *array_begin;

    for (int i = 0; i < begin_data.size; ++i) {
      if (!functor(i))
        return std::unexpected("Functor returned false");
    }

    auto* array_end = TRY(mReader.expect<RHSTReader::ArrayEndToken>());
    if (array_end == nullptr)
      return std::unexpected("Array terminator Result<void>");

    return {};
  }

  template <typename T> Result<void> dictIter(T functor) {
    auto* begin = TRY(mReader.expect<RHSTReader::DictBeginToken>());
    if (begin == nullptr)
      return std::unexpected("Expected a dictionary");

    // begin, the pointer, will be invalidated by future calls.
    auto begin_data = *begin;

    for (int i = 0; i < begin_data.size; ++i) {
      if (!functor(begin_data.name, i))
        return std::unexpected("std::unexpected");
    }

    auto* end = TRY(mReader.expect<RHSTReader::DictEndToken>());
    if (end == nullptr)
      return std::unexpected("std::unexpected");

    return {};
  }

  Result<void> read() {
    return dictIter(
        [&](std::string_view domain, int index) { return readRHSTSubNode(); });
  }
  Result<void> readRHSTSubNode() {
    return dictIter([&](std::string_view domain, int index) -> Result<void> {
      if (domain == "head") {
        return readMetaData();
      }
      if (domain == "body") {
        return readData();
      }

      return {};
    });
  }

  Result<void> readMetaData() {
    return dictIter([&](std::string_view key, int index) -> Result<void> {
      auto* value = TRY(mReader.expect<RHSTReader::StringToken>());

      if (value == nullptr)
        return std::unexpected("std::unexpected");

      if (key == "generator") {
        mBuilding.meta_data.exporter = value->data;
        return {};
      }
      if (key == "type") {
        mBuilding.meta_data.format = value->data;
        return {};
      }
      if (key == "version") {
        mBuilding.meta_data.exporter_version = value->data;
        return {};
      }
      if (key == "name") {
        return {};
      }

      return std::unexpected("Unsupported metadata trait");
    });
  }

  Result<void> readData() {
    return dictIter([&](std::string_view key, int index) -> Result<void> {
      if (key == "name") {
        auto ok = TRY(mReader.expectString());
        EXPECT(ok);
        return {};
      }

      if (key == "bones") {
        return readBones();
      }

      if (key == "polygons") {
        return readMeshes();
      }

      if (key == "materials") {
        return readMaterials();
      }

      // no-op for now
      if (key == "weights") {
        return readWeights();
      }

      return std::unexpected("Unknown section");
    });
  }

  Result<void> readWeights() {
    return arrayIter([&](int index) { return readWeight(index); });
  }

  Result<void> readWeight(int index) {
    WeightMatrix matrix{};
    // influences
    TRY(arrayIter([&](int index) -> Result<void> {
      // a single influence
      std::array<s32, 2> data;
      if (!arrayIter(
              [&](int index) -> Result<void> { return readInt(data[index]); }))
        return std::unexpected("Failed to read weight");

      matrix.weights.push_back(
          Weight{.bone_index = data[0], .influence = data[1]});
      return {};
    }));
    mBuilding.weights.push_back(matrix);

    return {};
  }

  Result<void> readBones() {
    return arrayIter([&](int index) { return readBone(index); });
  }

  Result<void> readBone(int index) {
    Bone bone{};
    TRY(stringKeyIter([&](std::string_view key, int index) -> Result<void> {
      if (key == "name") {
        auto* str = TRY(mReader.expect<RHSTReader::StringToken>());
        if (!str)
          return std::unexpected("Expected a string");

        bone.name = str->data;
        return {};
      }

      auto read_billboard_mode = [](const auto& str) {
        if (str == "y_face")
          return BillboardMode::Y_Face;
        if (str == "y_parallel")
          return BillboardMode::Y_Parallel;
        if (str == "z_face")
          return BillboardMode::Z_Face;
        if (str == "z_parallel")
          return BillboardMode::Z_Parallel;
        if (str == "zrotate_face")
          return BillboardMode::ZRotate_Face;
        if (str == "zrotate_parallel")
          return BillboardMode::ZRotate_Parallel;
        if (str == "none")
          return BillboardMode::None;
        return BillboardMode::None;
      };
      if (key == "billboard") {
        auto* tok = TRY(mReader.expect<RHSTReader::StringToken>());
        if (!tok)
          return std::unexpected("Expected a string");

        bone.billboard_mode = read_billboard_mode(tok->data);

        return {};
      }

      if (key == "parent") {
        return readInt(bone.parent);
      }

      if (key == "child") {
        int child = -1;
        TRY(readInt(child));
        if (child != -1) {
          bone.child.push_back(child);
        }
        return {};
      }

      if (key == "scale") {
        return readVec3(bone.scale);
      }
      if (key == "rotate") {
        return readVec3(bone.rotate);
      }
      if (key == "translate") {
        return readVec3(bone.translate);
      }

      if (key == "min") {
        return readVec3(bone.min);
      }
      if (key == "max") {
        return readVec3(bone.max);
      }

      if (key == "draws") {
        return arrayIter([&](int index) -> Result<void> {
          std::array<s32, 3> vals;
          if (!readIntArray(vals))
            return std::unexpected("Unable to read draw call");

          bone.draw_calls.push_back(DrawCall{
              .mat_index = vals[0], .poly_index = vals[1], .prio = vals[2]});

          return {};
        });
      }

      return std::unexpected("unexpected key");
    }));

    mBuilding.bones.push_back(bone);

    return {};
  }

  Result<void> readMaterials() {
    return arrayIter([&](int index) { return readMaterial(index); });
  }

  Result<void> readMaterial(int index) {
    ProtoMaterial material{};
    TRY(stringKeyIter([&](std::string_view key, int index) -> Result<void> {
      if (key == "name") {
        auto* str = TRY(mReader.expect<RHSTReader::StringToken>());
        if (!str)
          return std::unexpected("Expected a string");

        material.name = str->data;
        return {};
      }

      if (key == "texture") {
        return readString(material.texture_name);
      }

      auto wrap_mode_from_string = [](const auto& str) {
        if (str == "repeat")
          return WrapMode::Repeat;
        if (str == "mirror")
          return WrapMode::Mirror;
        if (str == "clamp")
          return WrapMode::Clamp;

        return WrapMode::Clamp;
      };
      if (key == "wrap_u") {
        auto* tok = TRY(mReader.expect<RHSTReader::StringToken>());
        if (!tok)
          return std::unexpected("Expected a string");
        material.wrap_u = wrap_mode_from_string(tok->data);
        return {};
      }
      if (key == "wrap_v") {
        auto* tok = TRY(mReader.expect<RHSTReader::StringToken>());
        if (!tok)
          return std::unexpected("Expected a string");
        material.wrap_v = wrap_mode_from_string(tok->data);
        return {};
      }
      if (key == "display_front") {
        auto* tok = TRY(mReader.expect<RHSTReader::S32Token>());
        if (!tok)
          return std::unexpected("Expected a s32");
        material.show_front = tok->data != 0;
        return {};
      }
      if (key == "display_back") {
        auto* tok = TRY(mReader.expect<RHSTReader::S32Token>());
        if (!tok)
          return std::unexpected("Expected a s32");
        material.show_back = tok->data != 0;
        return {};
      }
      if (key == "pe") {
        auto* tok = TRY(mReader.expect<RHSTReader::StringToken>());
        if (!tok)
          return std::unexpected("Expected a string");

        if (tok->data == "opaque") {
          material.alpha_mode = AlphaMode::Opaque;
          return {};
        } else if (tok->data == "outline") {
          material.alpha_mode = AlphaMode::Clip;
          return {};
        } else if (tok->data == "translucent") {
          material.alpha_mode = AlphaMode::Translucent;
          return {};
        }

        return std::unexpected("Invalid alpha mode");
      }
      if (key == "lightset") {
        return readInt(material.lightset_index);
      }
      if (key == "fog") {
        return readInt(material.fog_index);
      }

      if (key == "preset_path_mdl0mat") {
        return readString(material.preset_path_mdl0mat);
      }

      return std::unexpected("unexpected key");
    }));

    mBuilding.materials.push_back(material);

    return {};
  }

  Result<void> readMeshes() {
    return arrayIter([&](int index) { return readMesh(index); });
  }

  Result<void> readMesh(int index) {
    Mesh mesh{};
    TRY(stringKeyIter([&](std::string_view key, int index) -> Result<void> {
      if (key == "name") {
        auto* str = TRY(mReader.expect<RHSTReader::StringToken>());
        if (!str)
          return std::unexpected("Expected a string");

        mesh.name = str->data;
        return {};
      }

      if (key == "primitive_type") {
        TRY(mReader.readTok());
        return {};
      }

      if (key == "current_matrix") {
        return readInt(mesh.current_matrix);
      }

      if (key == "facepoint_format") {
        std::array<s32, 21> vcd;
        if (!readIntArray(vcd))
          return std::unexpected("Failed to read VCD");

        u32 vcd_bits = 0;
        for (int i = 0; i < vcd.size(); ++i) {
          vcd_bits |= vcd[i] != 0 ? (1 << i) : 0;
        }

        mesh.vertex_descriptor = vcd_bits;
        return {};
      }

      if (key == "matrix_primitives") {
        return readMatrixPrimitives(mesh.matrix_primitives,
                                    mesh.vertex_descriptor);
      }

      return std::unexpected("Unexpected value");
    }));

    mBuilding.meshes.push_back(mesh);

    return {};
  }

  Result<void> readMatrixPrimitives(std::vector<MatrixPrimitive>& out,
                                    u32 vcd) {
    MatrixPrimitive cur;
    TRY(arrayIter([&](int index) { return readMatrixPrimitive(cur, vcd); }));
    out.push_back(cur);
    return {};
  }

  Result<void> readMatrixPrimitive(MatrixPrimitive& out, u32 vcd) {
    return stringKeyIter([&](std::string_view key, int index) -> Result<void> {
      if (key == "name") {
        mReader.expect<RHSTReader::StringToken>();

        return {};
      }

      if (key == "matrix") {
        return readIntArray(out.draw_matrices);
      }

      if (key == "primitives") {
        return readPrimitives(out.primitives, vcd);
      }

      return std::unexpected("unexpected value");
    });
  }

  Result<void> readPrimitives(std::vector<Primitive>& out, u32 vcd) {
    Primitive cur;
    TRY(arrayIter([&](int index) { return readPrimitive(cur, vcd); }));
    out.push_back(cur);
    return {};
  }

  Result<void> readPrimitive(Primitive& out, u32 vcd) {
    return stringKeyIter([&](std::string_view key, int index) -> Result<void> {
      if (key == "name") {
        TRY(mReader.expect<RHSTReader::StringToken>());

        return {};
      }

      if (key == "primitive_type") {
        auto* prim_type = TRY(mReader.expect<RHSTReader::StringToken>());

        if (!prim_type)
          return std::unexpected("Expected a primitive type");

        if (prim_type->data == "triangles") {
          out.topology = Topology::Triangles;
          return {};
        }
        if (prim_type->data == "triangle_strips") {
          out.topology = Topology::TriangleStrip;
          return {};
        }
        if (prim_type->data == "triangle_fans") {
          out.topology = Topology::TriangleFan;
          return {};
        }

        return std::unexpected("Invalid topology type");
      }

      if (key == "facepoints") {
        return readVertices(out.vertices, vcd);
      }

      return std::unexpected("Unexpected value");
    });
  }

  Result<void> readVertices(std::vector<Vertex>& out, u32 vcd) {
    auto* begin = TRY(mReader.expect<RHSTReader::ArrayBeginToken>());
    if (!begin)
      return std::unexpected("Expeted array begin");
    out.resize(begin->size);
    for (auto& v : out) {
      if (!readVertex(v, vcd)) {
        return std::unexpected("Failed to read vert");
      }
    }
    auto* end = TRY(mReader.expect<RHSTReader::ArrayEndToken>());
    if (!end)
      return std::unexpected("Expected array end");

    return {};
  }

  Result<void> readVertex(Vertex& out, u32 vcd) {
    int vcd_cursor = 0; // LSB

    return arrayIter([&](int index) -> Result<void> {
      while ((vcd & (1 << vcd_cursor)) == 0) {
        ++vcd_cursor;

        if (vcd_cursor >= 21) {
          return std::unexpected("Missing vertex data");
        }
      }
      const int cur_attr = vcd_cursor;
      ++vcd_cursor;

      if (cur_attr == 9) {
        return readVec3(out.position);
      } else if (cur_attr == 10) {
        return readVec3(out.normal);
      } else if (cur_attr >= 11 && cur_attr <= 12) {
        const int color_index = cur_attr - 11;
        return readVec4(out.colors[color_index]);
      } else if (cur_attr >= 13 && cur_attr <= 20) {
        const int uv_index = cur_attr - 13;
        return readVec2(out.uvs[uv_index]);
      }

      return std::unexpected("unexpected value");
    });
  }

  Result<void> readString(std::string& out) {
    auto* val = TRY(mReader.expect<RHSTReader::StringToken>());
    if (!val)
      return std::unexpected("Expected a string");

    out = val->data;
    return {};
  }

  Result<void> readInt(s32& out) {
    auto* val = TRY(mReader.expect<RHSTReader::S32Token>());
    if (!val)
      return std::unexpected("Expected a S32");

    out = val->data;
    return {};
  }
  Result<void> readFloat(f32& out) {
    auto* val = TRY(mReader.expect<RHSTReader::F32Token>());
    if (!val)
      return std::unexpected("Expected a F32");

    out = val->data;
    return {};
  }
  Result<void> readNumber(f32& out) {
    TRY(mReader.readTok());
    auto tok = mReader.get();

    {
      auto* f32_val = rsl::dyn_cast<RHSTReader::F32Token>(tok);
      if (f32_val) {
        out = f32_val->data;
        return {};
      }
    }

    {
      auto* s32_val = rsl::dyn_cast<RHSTReader::S32Token>(tok);
      if (s32_val) {
        out = s32_val->data;
        return {};
      }
    }

    return std::unexpected("Expected a number");
  }
  Result<void> readVec2(glm::vec2& out) {
    return arrayIter(
        [&](int v_index) -> Result<void> { return readNumber(out[v_index]); });
  }
  Result<void> readVec3(glm::vec3& out) {
    return arrayIter(
        [&](int v_index) -> Result<void> { return readNumber(out[v_index]); });
  }
  Result<void> readVec4(glm::vec4& out) {
    return arrayIter(
        [&](int v_index) -> Result<void> { return readNumber(out[v_index]); });
  }

  template <size_t N> Result<void> readIntArray(std::array<s32, N>& out) {
    return arrayIter([&](int v_index) { return readInt(out[v_index]); });
  }

  template <typename T> Result<void> stringKeyIter(T functor) {
    return dictIter([&](std::string_view root, int index) -> Result<void> {
      auto* begin = TRY(mReader.expect<RHSTReader::DictBeginToken>());
      if (!begin)
        return std::unexpected("Expected a dictionary");

      auto begin_data = *begin;

      if (!functor(begin_data.name, index))
        return std::unexpected("Functor returned false");

      auto* end = TRY(mReader.expect<RHSTReader::DictEndToken>());
      if (!end)
        return std::unexpected("Expected a dictionary terminator");

      return {};
    });
  }

  SceneTree&& getResult() { return std::move(mBuilding); }

private:
  RHSTReader& mReader;
  SceneTree mBuilding;
};

class JsonSceneTreeReader {
public:
  JsonSceneTreeReader(const std::string& data)
      : json(nlohmann::json::parse(data)) {}
  SceneTree&& takeResult() { return std::move(out); }
  Result<void> read() {
    if (json.contains("head")) {
      auto head = json["head"];
      out.meta_data.exporter =
          get<std::string>(head, "generator").value_or("?");
      out.meta_data.format = get<std::string>(head, "type").value_or("?");
      if (out.meta_data.format != "JMDL2") {
        return std::unexpected("Blender plugin out of date. Please update.");
      }
      out.meta_data.exporter_version =
          get<std::string>(head, "version").value_or("?");
    }
    if (json.contains("body")) {
      auto body = json["body"];
      if (body.contains("name")) {
        out.name =
			get<std::string>(body, "name").value_or("course");
      }
      if (body.contains("bones") && body["bones"].is_array()) {
        auto bones = body["bones"];
        for (auto& bone : bones) {
          auto& b = out.bones.emplace_back();
          b.name = get<std::string>(bone, "name").value_or("?");
          std::string bill_mode =
              get<std::string>(bone, "billboard").value_or("None");
          b.billboard_mode =
              magic_enum::enum_cast<BillboardMode>(cap(bill_mode))
                  .value_or(BillboardMode::None);
          // Ignored: billboard
          b.parent = get<s32>(bone, "parent").value_or(-1);
          // We entirely recompute child links (from the "parent" field) and no
          // longer read the legacy "child" field
          b.scale = getVec3(bone, "scale").value_or(glm::vec3(1.0f));
          b.rotate = getVec3(bone, "rotate").value_or(glm::vec3(0.0f));
          b.translate = getVec3(bone, "translate").value_or(glm::vec3(0.0f));
          b.min = getVec3(bone, "min").value_or(glm::vec3(0.0f));
          b.max = getVec3(bone, "max").value_or(glm::vec3(0.0f));
          if (bone.contains("draws") && bone["draws"].is_array()) {
            auto draws = bone["draws"];
            for (auto& draw : draws) {
              int mat = draw[0].get<int>();
              int poly = draw[1].get<int>();
              int prio = draw[2].get<int>();
              b.draw_calls.push_back(
                  DrawCall{.mat_index = mat, .poly_index = poly, .prio = prio});
            }
          }
        }
      }
      if (body.contains("polygons") && body["polygons"].is_array()) {
        auto polygons = body["polygons"];
        for (auto& poly : polygons) {
          auto& b = out.meshes.emplace_back();
          b.name = get<std::string>(poly, "name").value_or("?");
          // Ignored: primitive_type
          b.current_matrix = get<s32>(poly, "current_matrix").value_or(-1);
          auto f = poly["facepoint_format"];
          b.vertex_descriptor = 0;
          for (s32 i = 0; i < 21; ++i) {
            b.vertex_descriptor |= f[i].get<s32>() ? (1 << i) : 0;
          }

          if (poly.contains("matrix_primitives") &&
              poly["matrix_primitives"].is_array()) {
            auto mps = poly["matrix_primitives"];
            for (auto& mp : mps) {
              auto& c = b.matrix_primitives.emplace_back();
              // Ignore name
              int i = 0;
              for (auto x : mp["matrix"]) {
                c.draw_matrices[i++] = x.get<s32>();
              }
              for (auto x : mp["primitives"]) {
                auto& d = c.primitives.emplace_back();
                // Ignore name
                auto t =
                    get<std::string>(x, "primitive_type").value_or("triangles");
                if (t == "triangles") {
                  d.topology = Topology::Triangles;
                } else if (t == "triangle_strips") {
                  d.topology = Topology::TriangleStrip;
                } else if (t == "triangle_fans") {
                  d.topology = Topology::TriangleFan;
                } else {
                  return std::unexpected(std::format("Unknown topology {}", t));
                }
                for (auto& v : x["facepoints"]) {
                  auto& e = d.vertices.emplace_back();
                  int vcd_cursor = 0; // LSB
                  int P = 0;
                  for (auto _ : v) {
                    while ((b.vertex_descriptor & (1 << vcd_cursor)) == 0) {
                      ++vcd_cursor;

                      if (vcd_cursor >= 21) {
                        return std::unexpected("Missing vertex data");
                      }
                    }
                    const int cur_attr = vcd_cursor;
                    ++vcd_cursor;

                    // PNMIDX
                    if (cur_attr == 0) {
                      e.matrix_index = v[P].get<s8>();
                    }
                    // TEXNMTXIDX are implicitly added by binary converter
                    else if (cur_attr == 9) {
                      e.position = getVec3(v, P).value_or(glm::vec3{});
                    } else if (cur_attr == 10) {
                      e.normal = getVec3(v, P).value_or(glm::vec3{});
                    } else if (cur_attr >= 11 && cur_attr <= 12) {
                      const int color_index = cur_attr - 11;
                      e.colors[color_index] =
                          getVec4(v, P).value_or(glm::vec4{});
                    } else if (cur_attr >= 13 && cur_attr <= 20) {
                      const int uv_index = cur_attr - 13;
                      e.uvs[uv_index] = getVec2(v, P).value_or(glm::vec2{});
                    }
                  }
                }
              }
            }
          }
        }
      }
      if (body.contains("weights") && body["weights"].is_array()) {
        auto weights = body["weights"];
        for (auto weight : weights) {
          auto& b = out.weights.emplace_back();
          for (auto influence : weight) {
            auto& c = b.weights.emplace_back();
            c.bone_index = influence[0].get<s32>();
            c.influence = influence[0].get<s32>();
          }
        }
      }
      if (body.contains("materials") && body["materials"].is_array()) {
        auto materials = body["materials"];
        for (auto mat : materials) {
          auto& b = out.materials.emplace_back();
          b.name = get<std::string>(mat, "name").value_or("?");
          b.texture_name = get<std::string>(mat, "texture").value_or("?");
          b.wrap_u =
              magic_enum::enum_cast<WrapMode>(
                  cap(get<std::string>(mat, "wrap_u").value_or("Repeat")))
                  .value_or(WrapMode::Repeat);
          b.wrap_v =
              magic_enum::enum_cast<WrapMode>(
                  cap(get<std::string>(mat, "wrap_v").value_or("Repeat")))
                  .value_or(WrapMode::Repeat);
          b.show_front = get<bool>(mat, "display_front").value_or(true);
          b.show_back = get<bool>(mat, "display_back").value_or(false);
          b.alpha_mode =
              magic_enum::enum_cast<AlphaMode>(
                  cap(get<std::string>(mat, "pe").value_or("Opaque")))
                  .value_or(AlphaMode::Opaque);
          if (b.alpha_mode == AlphaMode::Custom) {
            if (mat.contains("pe_settings")) {
              auto pe = mat["pe_settings"];
              b.pe.alpha_test = 
                    magic_enum::enum_cast<AlphaTest>(
                      cap(get<std::string>(pe, "alpha_test").value_or("Stencil")))
                      .value_or(AlphaTest::Stencil);
              // Alpha Test
              if(b.pe.alpha_test == AlphaTest::Custom) {
                b.pe.comparison_left =
                    magic_enum::enum_cast<Comparison>(
                        cap(get<std::string>(pe, "comparison_left").value_or("Always")))
                        .value_or(Comparison::Always);
                b.pe.comparison_right =
                    magic_enum::enum_cast<Comparison>(
                        cap(get<std::string>(pe, "comparison_right").value_or("Always")))
                        .value_or(Comparison::Always);
                b.pe.comparison_ref_left = get<u8>(pe, "comparison_ref_left").value_or(0);
                b.pe.comparison_ref_right = get<u8>(pe, "comparison_ref_right").value_or(0);
                b.pe.comparison_op =
                    magic_enum::enum_cast<AlphaOp>(
                      cap(get<std::string>(pe, "comparison_op").value_or("And")))
                      .value_or(AlphaOp::And);
              }
              // Draw Pass
              b.pe.xlu = get<bool>(pe, "xlu").value_or(false);
              // Z Buffer
              b.pe.z_early_comparison = get<bool>(pe, "z_early_compare").value_or(true);
              b.pe.z_compare = get<bool>(pe, "z_compare").value_or(true);
              b.pe.z_comparison = 
                      magic_enum::enum_cast<Comparison>(
                        cap(get<std::string>(pe, "z_comparison").value_or("LEqual")))
                        .value_or(Comparison::LEqual);
              b.pe.z_update = get<bool>(pe, "z_update").value_or(true);
			  // Dst Alpha
              b.pe.dst_alpha_enabled =
                  get<bool>(pe, "dst_alpha_enabled").value_or(false);
              b.pe.dst_alpha = get<u8>(pe, "dst_alpha").value_or(0);
			} else {
              b.alpha_mode = AlphaMode::Opaque;
			}
		  }

          b.lightset_index = get<s32>(mat, "lightset").value_or(-1);
          b.fog_index = get<s32>(mat, "fog").value_or(0);
          b.preset_path_mdl0mat =
              get<std::string>(mat, "preset_path_mdl0mat").value_or("");
          b.min_filter = get<bool>(mat, "min_filter").value_or(true);
          b.mag_filter = get<bool>(mat, "mag_filter").value_or(true);
          b.enable_mip = get<bool>(mat, "enable_mip").value_or(true);
          b.mip_filter = get<bool>(mat, "mip_filter").value_or(true);
          b.lod_bias = get<f32>(mat, "lod_bias").value_or(-1.0f);
        }
      }
    }
    return {};
  }

private:
  std::string cap(std::string s) {
    if (s.empty())
      return s;
    s[0] = toupper(s[0]);
    return s;
  }

  template <typename T> std::optional<T> get(auto& j, const std::string& name) {
    auto it = j.find(name);
    if (it == j.end()) {
      return std::nullopt;
    }
    return it->template get<T>();
  }
  std::optional<glm::vec3> getVec3(auto& j, const std::string& name) {
    auto it = j.find(name);
    if (it == j.end()) {
      return std::nullopt;
    }
    auto x = (*it)[0].template get<f32>();
    auto y = (*it)[1].template get<f32>();
    auto z = (*it)[2].template get<f32>();
    return glm::vec3(x, y, z);
  }
  std::optional<glm::vec2> getVec2(auto& jj, int& i) {
    auto j = jj[i++];
    auto x = (j)[0].template get<f32>();
    auto y = (j)[1].template get<f32>();
    return glm::vec2(x, y);
  }
  std::optional<glm::vec3> getVec3(auto& jj, int& i) {
    auto j = jj[i++];
    auto x = (j)[0].template get<f32>();
    auto y = (j)[1].template get<f32>();
    auto z = (j)[2].template get<f32>();
    return glm::vec3(x, y, z);
  }
  std::optional<glm::vec4> getVec4(auto& jj, int& i) {
    auto j = jj[i++];
    auto x = (j)[0].template get<f32>();
    auto y = (j)[1].template get<f32>();
    auto z = (j)[2].template get<f32>();
    auto w = (j)[3].template get<f32>();
    return glm::vec4(x, y, z, w);
  }

  nlohmann::json json;
  SceneTree out;
};

u64 totalStrippingMs = 0;

Result<SceneTree> ReadSceneTree(std::span<const u8> file_data) {
  totalStrippingMs = 0;
  if (file_data[0] == 'R' && file_data[1] == 'H' && file_data[2] == 'S' &&
      file_data[3] == 'T') {
    RHSTReader reader(file_data);

    SceneTreeReader scn_reader(reader);

    if (!scn_reader.read()) {
      return std::unexpected("Failed to read BINARY rhst scene tree");
    }

    return std::move(scn_reader.getResult());
  }
  std::string tmp(
      reinterpret_cast<const char*>(file_data.data()),
      reinterpret_cast<const char*>(file_data.data() + file_data.size()));
  JsonSceneTreeReader scn_reader(tmp);
  auto result = scn_reader.read();
  if (!result) {
    return std::unexpected(
        std::format("Failed to read JSON rhst scene tree: {}", result.error()));
  }
  SceneTree&& scn = scn_reader.takeResult();
  // Recompute child links
  for (auto&& bone : scn.bones) {
    bone.child.clear();
  }
  for (size_t i = 0; i < scn.bones.size(); ++i) {
    auto&& bone = scn.bones[i];
    if (bone.parent >= 0 && bone.parent < scn.bones.size()) {
      scn.bones[bone.parent].child.push_back(i);
    }
  }
  return std::move(scn_reader.takeResult());
}

} // namespace librii::rhst
