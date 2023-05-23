#include "RHST.hpp"
#include <oishii/reader/binary_reader.hxx>
#include <rsl/TaggedUnion.hpp>
#include <vendor/magic_enum/magic_enum.hpp>
#include <vendor/nlohmann/json.hpp>

IMPORT_STD;

namespace librii::rhst {

struct Expected {
  explicit Expected(bool v) : success(v) { assert(v); }

  operator bool() { return success; }

  bool success = false;
};

struct Failure {
  Failure(const char* msg) { printf("%s\n", msg); }

  operator Expected() { return Expected(false); }
};

struct Success {
  operator Expected() { return Expected(true); }
};

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

  void readTok() { mLastToken = _readTok(); }
  Token get() const { return mLastToken; }

  template <typename T> const T* expect() {
    readTok();
    return rsl::dyn_cast<T>(mLastToken);
  }

  bool expectSimple(RHSTToken type) {
    readTok();
    return mLastToken.getType() == type;
  }

  bool expectString() { return expectSimple(RHSTToken::RHST_DATA_STRING); }
  bool expectInt() { return expectSimple(RHSTToken::RHST_DATA_S32); }
  bool expectFloat() { return expectSimple(RHSTToken::RHST_DATA_F32); }

private:
  std::string_view _readString() {
    const u32 string_len = mReader.tryRead<u32>().value();
    const u32 start_start_pos = mReader.tell();
    mReader.skip(roundUp(string_len, 4));

    const char* string_begin =
        reinterpret_cast<const char*>(mReader.getStreamStart()) +
        start_start_pos;
    std::string_view data{string_begin, string_len};

    return data;
  }

  Token _readTok() {
    const RHSTToken type =
        static_cast<RHSTToken>(mReader.tryRead<s32>().value());

    switch (type) {
    case RHSTToken::RHST_DATA_STRING: {
      std::string_view data = _readString();

      return {StringToken{.data = data}};
    }
    case RHSTToken::RHST_DATA_S32: {
      const s32 data = mReader.tryRead<s32>().value();

      return {S32Token{.data = data}};
    }
    case RHSTToken::RHST_DATA_F32: {
      const f32 data = mReader.tryRead<f32>().value();

      return {F32Token{.data = data}};
    }
    case RHSTToken::RHST_DATA_DICT: {
      const u32 nchildren = mReader.tryRead<u32>().value();
      std::string_view name = _readString();

      return {DictBeginToken{.name = name, .size = nchildren}};
    }
    case RHSTToken::RHST_DATA_END_DICT: {
      return {DictEndToken{}};
    }
    case RHSTToken::RHST_DATA_ARRAY: {
      const u32 nchildren = mReader.tryRead<u32>().value();
      [[maybe_unused]] const u32 child_type = mReader.tryRead<u32>().value();

      return {ArrayBeginToken{.size = nchildren}};
    }
    case RHSTToken::RHST_DATA_END_ARRAY: {
      return {ArrayEndToken{}};
    }
    default:
    case RHSTToken::RHST_DATA_NULL: {
      return {NullToken{}};
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

  template <typename T> Expected arrayIter(T functor) {
    auto* array_begin = mReader.expect<RHSTReader::ArrayBeginToken>();
    if (array_begin == nullptr)
      return Failure("Expected an array");

    auto begin_data = *array_begin;

    for (int i = 0; i < begin_data.size; ++i) {
      if (!functor(i))
        return Failure("Functor returned false");
    }

    auto* array_end = mReader.expect<RHSTReader::ArrayEndToken>();
    if (array_end == nullptr)
      return Failure("Array terminator expected");

    return Success();
  }

  template <typename T> Expected dictIter(T functor) {
    auto* begin = mReader.expect<RHSTReader::DictBeginToken>();
    if (begin == nullptr)
      return Failure("Expected a dictionary");

    // begin, the pointer, will be invalidated by future calls.
    auto begin_data = *begin;

    for (int i = 0; i < begin_data.size; ++i) {
      if (!functor(begin_data.name, i))
        return Failure("Failure");
    }

    auto* end = mReader.expect<RHSTReader::DictEndToken>();
    if (end == nullptr)
      return Failure("Failure");

    return Success();
  }

  Expected read() {
    return dictIter(
        [&](std::string_view domain, int index) { return readRHSTSubNode(); });
  }
  Expected readRHSTSubNode() {
    return dictIter([&](std::string_view domain, int index) -> Expected {
      if (domain == "head") {
        return readMetaData();
      }
      if (domain == "body") {
        return readData();
      }

      return Success();
    });
  }

  Expected readMetaData() {
    return dictIter([&](std::string_view key, int index) -> Expected {
      auto* value = mReader.expect<RHSTReader::StringToken>();

      if (value == nullptr)
        return Failure("Failure");

      if (key == "generator") {
        mBuilding.meta_data.exporter = value->data;
        return Success();
      }
      if (key == "type") {
        mBuilding.meta_data.format = value->data;
        return Success();
      }
      if (key == "version") {
        mBuilding.meta_data.exporter_version = value->data;
        return Success();
      }
      if (key == "name") {
        return Success();
      }

      return Failure("Unsupported metadata trait");
    });
  }

  Expected readData() {
    return dictIter([&](std::string_view key, int index) -> Expected {
      if (key == "name") {
        return Expected(mReader.expectString());
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

      return Failure("Unknown section");
    });
  }

  Expected readWeights() {
    return arrayIter([&](int index) { return readWeight(index); });
  }

  Expected readWeight(int index) {
    WeightMatrix matrix{};
    // influences
    const bool res = arrayIter([&](int index) -> Expected {
      // a single influence
      std::array<s32, 2> data;
      if (!arrayIter(
              [&](int index) -> Expected { return readInt(data[index]); }))
        return Failure("Failed to read weight");

      matrix.weights.push_back(
          Weight{.bone_index = data[0], .influence = data[1]});
      return Success();
    });

    if (!res)
      return Failure("Failure");

    mBuilding.weights.push_back(matrix);

    return Success();
  }

  Expected readBones() {
    return arrayIter([&](int index) { return readBone(index); });
  }

  Expected readBone(int index) {
    Bone bone{};
    const bool res = stringKeyIter([&](std::string_view key,
                                       int index) -> Expected {
      if (key == "name") {
        auto* str = mReader.expect<RHSTReader::StringToken>();
        if (!str)
          return Failure("Expected a string");

        bone.name = str->data;
        return Success();
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
        auto* tok = mReader.expect<RHSTReader::StringToken>();
        if (!tok)
          return Expected("Expected a string");

        bone.billboard_mode = read_billboard_mode(tok->data);

        return Success();
      }

      if (key == "parent") {
        return readInt(bone.parent);
      }

      if (key == "child") {
        int child = -1;
        auto err = readInt(child);
        if (!err) {
          return err;
        }
        if (child != -1) {
          bone.child.push_back(child);
        }
        return Success();
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
        return arrayIter([&](int index) -> Expected {
          std::array<s32, 3> vals;
          if (!readIntArray(vals))
            return Failure("Unable to read draw call");

          bone.draw_calls.push_back(DrawCall{
              .mat_index = vals[0], .poly_index = vals[1], .prio = vals[2]});

          return Success();
        });
      }

      return Failure("Unexpected key");
    });

    if (!res)
      return Failure("Failure");

    mBuilding.bones.push_back(bone);

    return Success();
  }

  Expected readMaterials() {
    return arrayIter([&](int index) { return readMaterial(index); });
  }

  Expected readMaterial(int index) {
    ProtoMaterial material{};
    const bool res =
        stringKeyIter([&](std::string_view key, int index) -> Expected {
          if (key == "name") {
            auto* str = mReader.expect<RHSTReader::StringToken>();
            if (!str)
              return Failure("Expected a string");

            material.name = str->data;
            return Success();
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
            auto* tok = mReader.expect<RHSTReader::StringToken>();
            if (!tok)
              return Expected("Expected a string");
            material.wrap_u = wrap_mode_from_string(tok->data);
            return Success();
          }
          if (key == "wrap_v") {
            auto* tok = mReader.expect<RHSTReader::StringToken>();
            if (!tok)
              return Expected("Expected a string");
            material.wrap_v = wrap_mode_from_string(tok->data);
            return Success();
          }
          if (key == "display_front") {
            auto* tok = mReader.expect<RHSTReader::S32Token>();
            if (!tok)
              return Failure("Expected a s32");
            material.show_front = tok->data != 0;
            return Success();
          }
          if (key == "display_back") {
            auto* tok = mReader.expect<RHSTReader::S32Token>();
            if (!tok)
              return Failure("Expected a s32");
            material.show_back = tok->data != 0;
            return Success();
          }
          if (key == "pe") {
            auto* tok = mReader.expect<RHSTReader::StringToken>();
            if (!tok)
              return Failure("Expected a string");

            if (tok->data == "opaque") {
              material.alpha_mode = AlphaMode::Opaque;
              return Success();
            } else if (tok->data == "outline") {
              material.alpha_mode = AlphaMode::Clip;
              return Success();
            } else if (tok->data == "translucent") {
              material.alpha_mode = AlphaMode::Translucent;
              return Success();
            }

            return Failure("Invalid alpha mode");
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

          return Failure("Unexpected key");
        });

    if (!res)
      return Failure("Failure");

    mBuilding.materials.push_back(material);

    return Success();
  }

  Expected readMeshes() {
    return arrayIter([&](int index) { return readMesh(index); });
  }

  Expected readMesh(int index) {
    Mesh mesh{};
    const bool res =
        stringKeyIter([&](std::string_view key, int index) -> Expected {
          if (key == "name") {
            auto* str = mReader.expect<RHSTReader::StringToken>();
            if (!str)
              return Failure("Expected a string");

            mesh.name = str->data;
            return Success();
          }

          if (key == "primitive_type") {
            mReader.readTok();
            return Success();
          }

          if (key == "current_matrix") {
            return readInt(mesh.current_matrix);
          }

          if (key == "facepoint_format") {
            std::array<s32, 21> vcd;
            if (!readIntArray(vcd))
              return Failure("Failed to read VCD");

            u32 vcd_bits = 0;
            for (int i = 0; i < vcd.size(); ++i) {
              vcd_bits |= vcd[i] != 0 ? (1 << i) : 0;
            }

            mesh.vertex_descriptor = vcd_bits;
            return Success();
          }

          if (key == "matrix_primitives") {
            return readMatrixPrimitives(mesh.matrix_primitives,
                                        mesh.vertex_descriptor);
          }

          return Failure("Unexpected value");
        });

    if (!res)
      return Failure("Failure");

    mBuilding.meshes.push_back(mesh);

    return Success();
  }

  Expected readMatrixPrimitives(std::vector<MatrixPrimitive>& out, u32 vcd) {
    MatrixPrimitive cur;

    bool res =
        arrayIter([&](int index) { return readMatrixPrimitive(cur, vcd); });
    if (!res)
      return Failure("Failure");

    out.push_back(cur);
    return Success();
  }

  Expected readMatrixPrimitive(MatrixPrimitive& out, u32 vcd) {
    return stringKeyIter([&](std::string_view key, int index) -> Expected {
      if (key == "name") {
        mReader.expect<RHSTReader::StringToken>();

        return Success();
      }

      if (key == "matrix") {
        return readIntArray(out.draw_matrices);
      }

      if (key == "primitives") {
        return readPrimitives(out.primitives, vcd);
      }

      return Failure("Unexpected value");
    });
  }

  Expected readPrimitives(std::vector<Primitive>& out, u32 vcd) {
    Primitive cur;

    bool res = arrayIter([&](int index) { return readPrimitive(cur, vcd); });
    if (!res)
      return Failure("Failure");

    out.push_back(cur);
    return Success();
  }

  Expected readPrimitive(Primitive& out, u32 vcd) {
    return stringKeyIter([&](std::string_view key, int index) -> Expected {
      if (key == "name") {
        mReader.expect<RHSTReader::StringToken>();

        return Success();
      }

      if (key == "primitive_type") {
        auto* prim_type = mReader.expect<RHSTReader::StringToken>();

        if (!prim_type)
          return Failure("Expected a primitive type");

        if (prim_type->data == "triangles") {
          out.topology = Topology::Triangles;
          return Success();
        }
        if (prim_type->data == "triangle_strips") {
          out.topology = Topology::TriangleStrip;
          return Success();
        }
        if (prim_type->data == "triangle_fans") {
          out.topology = Topology::TriangleFan;
          return Success();
        }

        return Failure("Invalid topology type");
      }

      if (key == "facepoints") {
        return readVertices(out.vertices, vcd);
      }

      return Failure("Unexpected value");
    });
  }

  Expected readVertices(std::vector<Vertex>& out, u32 vcd) {
    auto* begin = mReader.expect<RHSTReader::ArrayBeginToken>();
    if (!begin)
      return Failure("Expeted array begin");
    out.resize(begin->size);
    for (auto& v : out) {
      if (!readVertex(v, vcd)) {
        return Failure("Failed to read vert");
      }
    }
    auto* end = mReader.expect<RHSTReader::ArrayEndToken>();
    if (!end)
      return Failure("Expeted array end");

    return Success();
  }

  Expected readVertex(Vertex& out, u32 vcd) {
    int vcd_cursor = 0; // LSB

    return arrayIter([&](int index) -> Expected {
      while ((vcd & (1 << vcd_cursor)) == 0) {
        ++vcd_cursor;

        if (vcd_cursor >= 21) {
          return Failure("Missing vertex data");
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

      return Failure("Unexpected value");
    });
  }

  Expected readString(std::string& out) {
    auto* val = mReader.expect<RHSTReader::StringToken>();
    if (!val)
      return Failure("Expected a string");

    out = val->data;
    return Success();
  }

  Expected readInt(s32& out) {
    auto* val = mReader.expect<RHSTReader::S32Token>();
    if (!val)
      return Failure("Expected a S32");

    out = val->data;
    return Success();
  }
  Expected readFloat(f32& out) {
    auto* val = mReader.expect<RHSTReader::F32Token>();
    if (!val)
      return Failure("Expected a F32");

    out = val->data;
    return Success();
  }
  Expected readNumber(f32& out) {
    mReader.readTok();
    auto tok = mReader.get();

    {
      auto* f32_val = rsl::dyn_cast<RHSTReader::F32Token>(tok);
      if (f32_val) {
        out = f32_val->data;
        return Success();
      }
    }

    {
      auto* s32_val = rsl::dyn_cast<RHSTReader::S32Token>(tok);
      if (s32_val) {
        out = s32_val->data;
        return Success();
      }
    }

    return Failure("Expected a number");
  }
  Expected readVec2(glm::vec2& out) {
    return arrayIter(
        [&](int v_index) -> Expected { return readNumber(out[v_index]); });
  }
  Expected readVec3(glm::vec3& out) {
    return arrayIter(
        [&](int v_index) -> Expected { return readNumber(out[v_index]); });
  }
  Expected readVec4(glm::vec4& out) {
    return arrayIter(
        [&](int v_index) -> Expected { return readNumber(out[v_index]); });
  }

  template <size_t N> Expected readIntArray(std::array<s32, N>& out) {
    return arrayIter([&](int v_index) { return readInt(out[v_index]); });
  }

  template <typename T> Expected stringKeyIter(T functor) {
    return dictIter([&](std::string_view root, int index) -> Expected {
      auto* begin = mReader.expect<RHSTReader::DictBeginToken>();
      if (!begin)
        return Failure("Expected a dictionary");

      auto begin_data = *begin;

      if (!functor(begin_data.name, index))
        return Failure("Functor returned false");

      auto* end = mReader.expect<RHSTReader::DictEndToken>();
      if (!end)
        return Failure("Expected a dictionary terminator");

      return Success();
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
              b.alpha_test = 
                    magic_enum::enum_cast<AlphaTest>(
                      cap(get<std::string>(pe, "alpha_test").value_or("Stencil")))
                      .value_or(AlphaTest::Stencil);
              if(b.alpha_test == AlphaTest::Custom) {
                b.comparison_left =
                    magic_enum::enum_cast<Comparison>(
                        cap(get<std::string>(pe, "comparison_left").value_or("Always")))
                        .value_or(Comparison::Always);
                b.comparison_right =
                    magic_enum::enum_cast<Comparison>(
                        cap(get<std::string>(pe, "comparison_right").value_or("Always")))
                        .value_or(Comparison::Always);
                b.comparison_ref_left = get<u8>(pe, "comparison_ref_left").value_or(0);
                b.comparison_ref_right = get<u8>(pe, "comparison_ref_right").value_or(0);
                b.comparison_op =
                    magic_enum::enum_cast<AlphaOp>(
                      cap(get<std::string>(pe, "comparison_op").value_or("And")))
                      .value_or(AlphaOp::And);
              }
              b.xlu = get<std::string>(pe, "draw_pass").value_or("opa") == "xlu";
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
