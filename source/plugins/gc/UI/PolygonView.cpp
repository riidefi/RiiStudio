#include <algorithm>
#include <core/3d/i3dmodel.hpp>
#include <core/util/gui.hpp>
#include <kpi/PropertyView.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace libcube::UI {

struct PolyDescriptorSurface final {
  const char* name = "Vertex Descriptor";
  const char* icon = ICON_FA_IMAGE;
};
struct PolyDataSurface final {
  const char* name = "Index Data";
  const char* icon = ICON_FA_IMAGE;
};

const char* vertexAttribNames =
    "PositionNormalMatrixIndex\0Texture0MatrixIndex\0Texture1MatrixIndex\0Tex"
    "ture2MatrixIndex\0Texture3MatrixIndex\0Texture4MatrixIndex\0Texture5Matr"
    "ixIndex\0Texture6MatrixIndex\0Texture7MatrixIndex\0Position\0Normal\0Col"
    "or0\0Color1\0TexCoord0\0TexCoord1\0TexCoord2\0TexCoord3\0TexCoord4\0TexC"
    "oord5\0TexCoord6\0TexCoord7\0PositionMatrixArray\0NormalMatrixArray\0Tex"
    "tureMatrixArray\0LightArray\0NormalBinormalTangent\0";
const char* vertexAttribNamesArray[] = {"PositionNormalMatrixIndex",
                                        "Texture0MatrixIndex",
                                        "Texture1MatrixIndex",
                                        "Texture2MatrixIndex",
                                        "Texture3MatrixIndex",
                                        "Texture4MatrixIndex",
                                        "Texture5MatrixIndex",
                                        "Texture6MatrixIndex",
                                        "Texture7MatrixIndex",
                                        "Position",
                                        "Normal",
                                        "Color0",
                                        "Color1",
                                        "TexCoord0",
                                        "TexCoord1",
                                        "TexCoord2",
                                        "TexCoord3",
                                        "TexCoord4",
                                        "TexCoord5",
                                        "TexCoord6",
                                        "TexCoord7",
                                        "PositionMatrixArray",
                                        "NormalMatrixArray",
                                        "TextureMatrixArray",
                                        "LightArray",
                                        "NormalBinormalTangent"};
void drawProperty(kpi::PropertyDelegate<IndexedPolygon> dl,
                  PolyDescriptorSurface) {
  auto& poly = dl.getActive();

  auto& desc = poly.getVcd();

  int i = 0;
  for (auto& attrib : desc.mAttributes) {
    riistudio::util::IDScope g(i++);

    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() / 3);

    int type = static_cast<int>(attrib.first);
    ImGui::Combo("Attribute Type", &type, vertexAttribNames);
    int format = static_cast<int>(attrib.second);
    ImGui::SameLine();
    ImGui::Combo("Attribute Format", &format,
                 "None\0Direct\0U8 / 8-bit / 0-255\0U16 / 16-bit / 0-65535\0");

    ImGui::PopItemWidth();
  }
}

void drawProperty(kpi::PropertyDelegate<IndexedPolygon> dl, PolyDataSurface) {
  auto& poly = dl.getActive();
  auto& desc = poly.getVcd();

  auto draw_p = [&](int i, int j) {
    auto prim = poly.getMatrixPrimitiveIndexedPrimitive(i, j);
    u32 k = 0;
    for (auto& v : prim.mVertices) {
      ImGui::TableNextRow();

      riistudio::util::IDScope v_s(k);

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%u", k);

      u32 q = 0;
      for (auto& e : poly.getVcd().mAttributes) {
        if (e.second == gx::VertexAttributeType::None)
          continue;
        int type = static_cast<int>(e.first);
        ImGui::TableSetColumnIndex(2 + q);
        int data = v.operator[](e.first);
        ImGui::Text("%i", data);
        ++q;
      }
      ++k;
    }
  };

  auto draw_mp = [&](int i) {
    ImGui::Text("Default Matrix: %u",
                (u32)poly.getMatrixPrimitiveCurrentMatrix(i));

    const int attrib_cnt = std::count_if(
        desc.mAttributes.begin(), desc.mAttributes.end(), [](const auto& e) {
          return e.second != gx::VertexAttributeType::None;
        });

    const auto table_flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
        ImGuiTableFlags_Sortable;
    if (ImGui::BeginTable("Vertex data", 2 + attrib_cnt, table_flags)) {
      u32 q = 0;
      ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("Primitive Index");
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("Vertex Index");
      for (auto& e : poly.getVcd().mAttributes) {
        if (e.second == gx::VertexAttributeType::None)
          continue;

        ImGui::TableSetColumnIndex(2 + q);

        int type = static_cast<int>(e.first);
        ImGui::Text(vertexAttribNamesArray[type]);
        ++q;
      }
      static const std::array<std::string, 8> prim_types{
          "Quads",        "QuadStrips", "Triangles",  "TriangleStrips",
          "TriangleFans", "Lines",      "LineStrips", "Points"};

      for (int j = 0; j < poly.getMatrixPrimitiveNumIndexedPrimitive(i); ++j) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        bool open = ImGui::TreeNodeEx(
            (std::string("#") + std::to_string(j) + " (" +
             prim_types[static_cast<int>(
                 poly.getMatrixPrimitiveIndexedPrimitive(i, j).mType)] +
             ")")
                .c_str(),
            ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen);
        if (open) {
          draw_p(i, j);
          ImGui::TreePop();
        }
      }

      ImGui::EndTable();
    }
  };

  if (ImGui::BeginTabBar("Matrix Primitives")) {
    for (int i = 0; i < poly.getNumMatrixPrimitives(); ++i) {
      if (ImGui::BeginTabItem(
              (std::string("Matrix Prim: ") + std::to_string(i)).c_str())) {
        draw_mp(i);
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
}

void installPolygonView() {
  kpi::PropertyViewManager& manager = kpi::PropertyViewManager::getInstance();
  manager.addPropertyView<libcube::IndexedPolygon, PolyDescriptorSurface>();
  manager.addPropertyView<libcube::IndexedPolygon, PolyDataSurface>();
}

} // namespace libcube::UI
