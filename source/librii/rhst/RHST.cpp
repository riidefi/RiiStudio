#include "RHST.hpp"
#include <rsl/TaggedUnion.hpp>
#include <vendor/magic_enum/magic_enum.hpp>
#include <vendor/nlohmann/json.hpp>

namespace librii::rhst {

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
        out.name = get<std::string>(body, "name").value_or("course");
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
          b.show_front = get<bool>(mat, "display_front").value_or(true);
          b.show_back = get<bool>(mat, "display_back").value_or(false);
          b.alpha_mode =
              magic_enum::enum_cast<AlphaMode>(
                  cap(get<std::string>(mat, "pe").value_or("Opaque")))
                  .value_or(AlphaMode::Opaque);
          if (b.alpha_mode == AlphaMode::Custom) {
            if (mat.contains("pe_settings")) {
              auto pe = mat["pe_settings"];
              b.pe.alpha_test = magic_enum::enum_cast<AlphaTest>(
                                    cap(get<std::string>(pe, "alpha_test")
                                            .value_or("Stencil")))
                                    .value_or(AlphaTest::Stencil);
              // Alpha Test
              if (b.pe.alpha_test == AlphaTest::Custom) {
                b.pe.comparison_left =
                    magic_enum::enum_cast<Comparison>(
                        cap(get<std::string>(pe, "comparison_left")
                                .value_or("Always")))
                        .value_or(Comparison::Always);
                b.pe.comparison_right =
                    magic_enum::enum_cast<Comparison>(
                        cap(get<std::string>(pe, "comparison_right")
                                .value_or("Always")))
                        .value_or(Comparison::Always);
                b.pe.comparison_ref_left =
                    get<u8>(pe, "comparison_ref_left").value_or(0);
                b.pe.comparison_ref_right =
                    get<u8>(pe, "comparison_ref_right").value_or(0);
                b.pe.comparison_op =
                    magic_enum::enum_cast<AlphaOp>(
                        cap(get<std::string>(pe, "comparison_op")
                                .value_or("And")))
                        .value_or(AlphaOp::And);
              }
              // Draw Pass
              b.pe.xlu = get<bool>(pe, "xlu").value_or(false);
              // Z Buffer
              b.pe.z_early_comparison =
                  get<bool>(pe, "z_early_compare").value_or(true);
              b.pe.z_compare = get<bool>(pe, "z_compare").value_or(true);
              b.pe.z_comparison = magic_enum::enum_cast<Comparison>(
                                      cap(get<std::string>(pe, "z_comparison")
                                              .value_or("LEqual")))
                                      .value_or(Comparison::LEqual);
              b.pe.z_update = get<bool>(pe, "z_update").value_or(true);
              // Dst Alpha
              b.pe.dst_alpha_enabled =
                  get<bool>(pe, "dst_alpha_enabled").value_or(false);
              b.pe.dst_alpha = get<u8>(pe, "dst_alpha").value_or(0);
              // Blend Modes
              b.pe.blend_type =
                  magic_enum::enum_cast<BlendModeType>(
                      cap(get<std::string>(pe, "blend_mode").value_or("None")))
                      .value_or(BlendModeType::None);
              b.pe.blend_source = magic_enum::enum_cast<BlendModeFactor>(
                                      cap(get<std::string>(pe, "blend_source")
                                              .value_or("Src_a")))
                                      .value_or(BlendModeFactor::Src_a);
              b.pe.blend_dest = magic_enum::enum_cast<BlendModeFactor>(
                                    cap(get<std::string>(pe, "blend_dest")
                                            .value_or("Inv_rc_a")))
                                    .value_or(BlendModeFactor::Inv_src_a);
            } else {
              b.alpha_mode = AlphaMode::Opaque;
            }
          }

          b.lightset_index = get<s32>(mat, "lightset").value_or(-1);
          b.fog_index = get<s32>(mat, "fog").value_or(0);
          b.preset_path_mdl0mat =
              get<std::string>(mat, "preset_path_mdl0mat").value_or("");

          // Swap Table
          if (mat.contains("swap_table") && mat["swap_table"].is_array()) {
            auto swap = mat["swap_table"];
            int i = 0;
            for (auto entry : swap) {
              b.swap_table[i].r =
                  magic_enum::enum_cast<Colors>(
                      cap(get<std::string>(entry, "red").value_or("Red")))
                      .value_or(Colors::Red);
              b.swap_table[i].g =
                  magic_enum::enum_cast<Colors>(
                      cap(get<std::string>(entry, "green").value_or("Green")))
                      .value_or(Colors::Green);
              b.swap_table[i].b =
                  magic_enum::enum_cast<Colors>(
                      cap(get<std::string>(entry, "blue").value_or("Blue")))
                      .value_or(Colors::Blue);
              b.swap_table[i].a =
                  magic_enum::enum_cast<Colors>(
                      cap(get<std::string>(entry, "alpha").value_or("Alpha")))
                      .value_or(Colors::Alpha);
              i++;
            }
          }

          // TEV
          if (mat.contains("tev") && mat["tev"].is_array()) {
            auto tevs = mat["tev"];
            if (tevs.size() > 16) {
              rsl::error("Too many TEV Stages: {} (Max 16)", tevs.size());
            } else {
              for (auto st : tevs) {
                auto& stage = b.tev_stages.emplace_back();

                stage.ras_channel =
                    magic_enum::enum_cast<ColorSelChan>(
                        get<std::string>(st, "channel").value_or("color0a0"))
                        .value_or(ColorSelChan::color0a0);
                stage.tex_map = get<u8>(st, "sampler").value_or(0);
                stage.ras_swap = get<u8>(st, "ras_swap").value_or(0);
                stage.tex_map_swap = get<u8>(st, "sampler_swap").value_or(0);

                auto c_stage = stage.color_stage;
                c_stage.constant_sel =
                    magic_enum::enum_cast<TevKColorSel>(
                        get<std::string>(st, "c_konst").value_or("const_1_8"))
                        .value_or(TevKColorSel::const_1_8);
                c_stage.formula =
                    magic_enum::enum_cast<TevColorOp>(
                        get<std::string>(st, "c_formula").value_or("add"))
                        .value_or(TevColorOp::add);
                c_stage.a =
                    magic_enum::enum_cast<TevColorArg>(
                        get<std::string>(st, "c_sel_a").value_or("zero"))
                        .value_or(TevColorArg::zero);
                c_stage.b =
                    magic_enum::enum_cast<TevColorArg>(
                        get<std::string>(st, "c_sel_b").value_or("rasc"))
                        .value_or(TevColorArg::rasc);
                c_stage.c =
                    magic_enum::enum_cast<TevColorArg>(
                        get<std::string>(st, "c_sel_c").value_or("texc"))
                        .value_or(TevColorArg::texc);
                c_stage.d =
                    magic_enum::enum_cast<TevColorArg>(
                        get<std::string>(st, "c_sel_d").value_or("zero"))
                        .value_or(TevColorArg::zero);
                c_stage.bias =
                    magic_enum::enum_cast<TevBias>(
                        get<std::string>(st, "c_bias").value_or("zero"))
                        .value_or(TevBias::zero);
                c_stage.scale =
                    magic_enum::enum_cast<TevScale>(
                        get<std::string>(st, "c_scale").value_or("scale_1"))
                        .value_or(TevScale::scale_1);
                c_stage.out =
                    magic_enum::enum_cast<TevReg>(
                        get<std::string>(st, "c_out").value_or("reg3"))
                        .value_or(TevReg::reg3);
                c_stage.clamp = get<bool>(st, "c_output_clamp").value_or(true);
                stage.color_stage = c_stage;

                auto a_stage = stage.alpha_stage;
                a_stage.constant_sel =
                    magic_enum::enum_cast<TevKAlphaSel>(
                        get<std::string>(st, "a_konst").value_or("const_1_8"))
                        .value_or(TevKAlphaSel::const_1_8);
                a_stage.formula =
                    magic_enum::enum_cast<TevAlphaOp>(
                        get<std::string>(st, "a_formula").value_or("add"))
                        .value_or(TevAlphaOp::add);
                a_stage.a =
                    magic_enum::enum_cast<TevAlphaArg>(
                        get<std::string>(st, "a_sel_a").value_or("zero"))
                        .value_or(TevAlphaArg::zero);
                a_stage.b =
                    magic_enum::enum_cast<TevAlphaArg>(
                        get<std::string>(st, "a_sel_b").value_or("rasa"))
                        .value_or(TevAlphaArg::rasa);
                a_stage.c =
                    magic_enum::enum_cast<TevAlphaArg>(
                        get<std::string>(st, "a_sel_c").value_or("texa"))
                        .value_or(TevAlphaArg::texa);
                a_stage.d =
                    magic_enum::enum_cast<TevAlphaArg>(
                        get<std::string>(st, "a_sel_d").value_or("zero"))
                        .value_or(TevAlphaArg::zero);
                a_stage.bias =
                    magic_enum::enum_cast<TevBias>(
                        get<std::string>(st, "a_bias").value_or("zero"))
                        .value_or(TevBias::zero);
                a_stage.scale =
                    magic_enum::enum_cast<TevScale>(
                        get<std::string>(st, "a_scale").value_or("scale_1"))
                        .value_or(TevScale::scale_1);
                a_stage.out =
                    magic_enum::enum_cast<TevReg>(
                        get<std::string>(st, "a_out").value_or("reg3"))
                        .value_or(TevReg::reg3);
                a_stage.clamp = get<bool>(st, "a_output_clamp").value_or(true);
                stage.alpha_stage = a_stage;
              }
            }
          }

          if (mat.contains("samplers") && mat["samplers"].is_array()) {
            auto samplers = mat["samplers"];
            for (auto sam : samplers) {
              auto& sampler = b.samplers.emplace_back();
              sampler.texture_name =
                  get<std::string>(sam, "texture").value_or("?");
              sampler.wrap_u =
                  magic_enum::enum_cast<WrapMode>(
                      cap(get<std::string>(sam, "wrap_u").value_or("Repeat")))
                      .value_or(WrapMode::Repeat);
              sampler.wrap_v =
                  magic_enum::enum_cast<WrapMode>(
                      cap(get<std::string>(sam, "wrap_v").value_or("Repeat")))
                      .value_or(WrapMode::Repeat);
              sampler.min_filter = get<bool>(sam, "min_filter").value_or(true);
              sampler.mag_filter = get<bool>(sam, "mag_filter").value_or(true);
              sampler.enable_mip = get<bool>(sam, "enable_mip").value_or(true);
              sampler.mip_filter = get<bool>(sam, "mip_filter").value_or(true);
              sampler.lod_bias = get<f32>(sam, "lod_bias").value_or(-1.0f);
              sampler.mapping =
                  magic_enum::enum_cast<Mapping>(
                      cap(get<std::string>(sam, "mapping").value_or("UVMap")))
                      .value_or(Mapping::UVMap);
              sampler.uv_map_index =
                  get<int>(sam, "mapping_uv_index").value_or(0);
              sampler.light_index =
                  get<int>(sam, "mapping_light_index").value_or(-1);
              sampler.camera_index =
                  get<int>(sam, "mapping_cam_index").value_or(-1);

              // Matrix
              if (sam.contains("transformations")) {
                auto trans = sam["transformations"];
                sampler.scale =
                    getVec2(trans, "scale").value_or(glm::vec2(1.0f));
                sampler.rotate = get<f32>(trans, "rotate").value_or(0.0f);
                sampler.trans =
                    getVec2(trans, "translate").value_or(glm::vec2(0.0f));
              }
            }
          } else {
            b.min_filter = get<bool>(mat, "min_filter").value_or(true);
            b.mag_filter = get<bool>(mat, "mag_filter").value_or(true);
            b.enable_mip = get<bool>(mat, "enable_mip").value_or(true);
            b.mip_filter = get<bool>(mat, "mip_filter").value_or(true);
            b.lod_bias = get<f32>(mat, "lod_bias").value_or(-1.0f);
            b.texture_name = get<std::string>(mat, "texture").value_or("?");
            b.wrap_u =
                magic_enum::enum_cast<WrapMode>(
                    cap(get<std::string>(mat, "wrap_u").value_or("Repeat")))
                    .value_or(WrapMode::Repeat);
            b.wrap_v =
                magic_enum::enum_cast<WrapMode>(
                    cap(get<std::string>(mat, "wrap_v").value_or("Repeat")))
                    .value_or(WrapMode::Repeat);
          }
          // TEV Colors
          if (mat.contains("tev_colors") && mat["tev_colors"].is_array()) {
            auto cols = mat["tev_colors"];
            int P = 0;
            b.tev_colors[0] = {0xaa, 0xbb, 0xcc, 0xff};
            for (int i = 0; i < 3; i++) {
              b.tev_colors[i + 1] = getVec4(cols, P).value_or(glm::vec4{});
            }
          }
          if (mat.contains("tev_konst_colors") &&
              mat["tev_konst_colors"].is_array()) {
            auto cols = mat["tev_konst_colors"];
            int P = 0;
            for (int i = 0; i < 4; i++) {
              b.tev_konst_colors[i] = getVec4(cols, P).value_or(glm::vec4{});
            }
          }
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

  std::optional<glm::vec2> getVec2(auto& j, const std::string& name) {
    auto it = j.find(name);
    if (it == j.end()) {
      return std::nullopt;
    }
    auto x = (*it)[0].template get<f32>();
    auto y = (*it)[1].template get<f32>();
    return glm::vec2(x, y);
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
    return std::unexpected("BINARY rhst scene tree unsupported");
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
