#include "MaterialView.hpp"

namespace riistudio::j3d::ui {

void drawProperty(kpi::PropertyDelegate<Material>& delegate, J3DDataSurface) {
  int flag = delegate.getActive().flag;
  ImGui::InputInt("Flag"_j, &flag, 1, 1);
  KPI_PROPERTY_EX(delegate, flag, static_cast<u8>(flag));

  librii::gx::ColorF32 clr_f32;

  if (ImGui::CollapsingHeader("Fog"_j, ImGuiTreeNodeFlags_DefaultOpen)) {
    auto& fog = delegate.getActive().fogInfo;

    int orthoOrPersp =
        static_cast<int>(fog.type) >=
                static_cast<int>(librii::gx::FogType::OrthographicLinear)
            ? 0
            : 1;
    int fogType = static_cast<int>(fog.type);
    if (fogType >= static_cast<int>(librii::gx::FogType::OrthographicLinear))
      fogType -= 5;
    ImGui::Combo("Projection"_j, &orthoOrPersp,
                 "Orthographic\0"
                 "Perspective\0"_j);
    ImGui::Combo("Function"_j, &fogType,
                 "None\0"
                 "Linear\0"
                 "Exponential\0"
                 "Quadratic\0"
                 "Inverse Exponential\0"
                 "Quadratic\0"_j);
    int new_type = fogType;
    if (new_type != 0)
      new_type += (1 - orthoOrPersp) * 5;
    KPI_PROPERTY_EX(delegate, fogInfo.type,
                    static_cast<librii::gx::FogType>(new_type));
    bool enabled = fog.enabled;
    ImGui::Checkbox("Fog Enabled"_j, &enabled);
    KPI_PROPERTY_EX(delegate, fogInfo.enabled, enabled);

    {
      util::ConditionalActive g(enabled /* && new_type != 0*/);
      ImGui::PushItemWidth(200);
      {
        int center = fog.center;
        ImGui::InputInt("Center"_j, &center);
        KPI_PROPERTY_EX(delegate, fogInfo.center, static_cast<u16>(center));

        float startZ = fog.startZ;
        ImGui::InputFloat("Start Z"_j, &startZ);
        KPI_PROPERTY_EX(delegate, fogInfo.startZ, startZ);
        ImGui::SameLine();
        float endZ = fog.endZ;
        ImGui::InputFloat("End Z"_j, &endZ);
        KPI_PROPERTY_EX(delegate, fogInfo.endZ, endZ);

        float nearZ = fog.nearZ;
        ImGui::InputFloat("Near Z"_j, &nearZ);
        KPI_PROPERTY_EX(delegate, fogInfo.nearZ, nearZ);
        ImGui::SameLine();
        float farZ = fog.farZ;
        ImGui::InputFloat("Far Z"_j, &farZ);
        KPI_PROPERTY_EX(delegate, fogInfo.farZ, farZ);
      }
      ImGui::PopItemWidth();
      clr_f32 = fog.color;
      ImGui::ColorEdit4("Fog Color"_j, clr_f32);
      KPI_PROPERTY_EX(delegate, fogInfo.color, (librii::gx::Color)clr_f32);

      // RangeAdjTable? Maybe a graph?
    }
  }

  if (ImGui::CollapsingHeader("Light Colors (Usually ignored by games)"_j,
                              ImGuiTreeNodeFlags_DefaultOpen)) {

    int i = 0;
    for (auto& clr : delegate.getActive().lightColors) {
      clr_f32 = clr;
      ImGui::ColorEdit4(
          (std::string("Light Color "_j) + std::to_string(i)).c_str(), clr_f32);

      KPI_PROPERTY(delegate, clr, (librii::gx::Color)clr_f32, lightColors[i]);
      ++i;
    }
  }
  if (ImGui::CollapsingHeader("NBT Scale"_j, ImGuiTreeNodeFlags_DefaultOpen)) {
    bool enabled = delegate.getActive().nbtScale.enable;
    auto scale = delegate.getActive().nbtScale.scale;

    ImGui::Checkbox("NBT Enabled"_j, &enabled);
    KPI_PROPERTY_EX(delegate, nbtScale.enable, enabled);

    {
      util::ConditionalActive g(enabled);

      ImGui::InputFloat3("Scale"_j, &scale.x);
      KPI_PROPERTY_EX(delegate, nbtScale.scale, scale);
    }
  }
}

void drawProperty(kpi::PropertyDelegate<Joint>& delegate, BoneJ3DSurface) {
  auto& bone = delegate.getActive();

  int flag = bone.flag;
  ImGui::InputInt("Flag"_j, &flag);
  KPI_PROPERTY_EX(delegate, flag, static_cast<u16>(flag));

  auto bbMtx = imcxx::Combo("Billboard Matrix"_j, bone.bbMtxType,
                            "Standard\0"
                            "XY\0"
                            "Y\0"_j);
  KPI_PROPERTY_EX(delegate, bbMtxType, bbMtx);

  auto boundingBox = bone.boundingBox;
  auto boundingSphereRadius = bone.boundingSphereRadius;
  Toolkit::BoundingVolume(&boundingBox, &boundingSphereRadius);
  KPI_PROPERTY_EX(delegate, boundingBox, boundingBox);
  KPI_PROPERTY_EX(delegate, boundingSphereRadius, boundingSphereRadius);

  const riistudio::lib3d::Model* pMdl =
      dynamic_cast<const riistudio::lib3d::Model*>(
          dynamic_cast<const kpi::IObject*>(&bone)->childOf);
  const auto mtx = calcSrtMtxSimple(delegate.getActive(), pMdl);

  ImGui::TextUnformatted("Computed Matrix:"_j);
  Toolkit::Matrix44(mtx);
}

void drawProperty(kpi::PropertyDelegate<Shape>& dl, ShapeJ3DSurface) {
  auto& shape = dl.getActive();

  auto mode = imcxx::Combo("Mode"_j, shape.mode,
                           "Standard\0"
                           "Billboard XY\0"
                           "Billboard Y\0"
                           "Skinned\0"_j);
  KPI_PROPERTY_EX(dl, mode, mode);

  auto bbox = shape.bbox;
  auto bsphere = shape.bsphere;
  Toolkit::BoundingVolume(&bbox, &bsphere);
  KPI_PROPERTY_EX(dl, bbox, bbox);
  KPI_PROPERTY_EX(dl, bsphere, bsphere);

  bool vis = shape.visible;
  ImGui::Checkbox("Visible"_j, &vis);
  KPI_PROPERTY_EX(dl, visible, vis);
  int i = 0;
  for (auto& mp : shape.mMatrixPrimitives) {
    ImGui::Text("Matrix Primitive: %i"_j, i);

    (void)mp;
#if 0
    const auto matrices =
        shape.getPosMtx(*dynamic_cast<libcube::Model*>(shape.childOf), i);
    int j = 0;
    for (auto& elem : mp.mDrawMatrixIndices) {
      ImGui::Text("DRW %i: %i"_j, j, elem);
      Toolkit::Matrix44(matrices[j]);
      ++j;
    }
    ++i;
#endif
  }
}

void drawProperty(kpi::PropertyDelegate<j3d::Model>& dl, ModelJ3DSurface) {
  auto& mdl = dl.getActive();

  int sclRule = static_cast<int>(mdl.mScalingRule);
  ImGui::Combo("Scaling Rule"_j, &sclRule,
               "Basic\0"
               "XSI\0"
               "Maya\0"_j);
  KPI_PROPERTY_EX(dl, mScalingRule,
                  static_cast<librii::j3d::ScalingRule>(sclRule));

  if (ImGui::CollapsingHeader("Draw Matrices / Envelopes"_j,
                              ImGuiTreeNodeFlags_DefaultOpen) &&
      ImGui::BeginChild("Entries"_j)) {
    int i = 0;
    for (const auto& drw : mdl.mDrawMatrices) {
      char buf[256];
      snprintf(buf, sizeof(buf), "Matrix %i (Total %i)"_j, i++,
               drw.mWeights.size());
      if (ImGui::CollapsingHeader(buf)) {
        util::IDScope g_i(i);
        int j = 0;
        for (const auto& w : drw.mWeights) {
          if (ImGui::CollapsingHeader(
                  (std::string("Weight "_j) + std::to_string(j++)).c_str()),
              ImGuiTreeNodeFlags_DefaultOpen) {
            util::IDScope g_j(j);
            int boneId = w.boneId;
            float weight = w.weight;
            ImGui::InputInt("Bone"_j, &boneId);
            ImGui::SameLine();
            ImGui::InputFloat("Influence"_j, &weight);
          }
        }
      }
    }

    ImGui::EndChild();
  }
}

kpi::DecentralizedInstaller Installer([](kpi::ApplicationPlugins&) {
  auto& inst = kpi::PropertyViewManager::getInstance();
  inst.addPropertyView<Joint, BoneJ3DSurface>();
  inst.addPropertyView<Shape, ShapeJ3DSurface>();
  inst.addPropertyView<Model, ModelJ3DSurface>();
});

} // namespace riistudio::j3d::ui
