#include "Indirect.hpp"

namespace libcube::UI {

using namespace riistudio::util;

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  IndirectSurface) {
  auto& matData = delegate.getActive().getMaterialData();
  const libcube::Scene* pScn = dynamic_cast<const libcube::Scene*>(
      dynamic_cast<const kpi::IObject*>(&delegate.getActive())
          ->childOf->childOf);
  {
    auto& stages = matData.indirectStages;

    std::array<bool, 4> opened{};

    if (ImGui::Button("Add a Configuration"_j)) {
      stages.push_back({});
      delegate.commit("Added Indirect Configuration");
    }
    if (ImGui::BeginTabBar("Configurations"_j,
                           ImGuiTabBarFlags_AutoSelectNewTabs |
                               ImGuiTabBarFlags_FittingPolicyResizeDown)) {
      for (int i = 0; i < stages.size(); ++i) {
        opened[i] = true;
        auto title = std::string("Configuration "_j) + std::to_string(i);
        if (ImGui::BeginTabItem(title.c_str(), &opened[i])) {
          auto& conf = matData.indirectStages[i];
          if (conf.order.refCoord != conf.order.refMap) {
            ImGui::TextUnformatted("Invalid configuration"_j);
          } else {
            const int texid =
                SamplerCombo(conf.order.refMap, matData.samplers,
                             pScn->getTextures(), delegate.mEd, true);
            if (texid != conf.order.refMap) {
              for (auto* e : delegate.mAffected) {
                auto& order = e->getMaterialData().indirectStages[i].order;
                order.refMap = order.refCoord = texid;
              }
              delegate.commit("Property update");
            }
            const char* scale_opt = "1x\0"
                                    "1/2x\0"
                                    "1/4x\0"
                                    "1/8x\0"
                                    "1/16x\0"
                                    "1/32x\0"
                                    "1/64x\0"
                                    "1/128x\0"
                                    "1/256x\0";

            auto u_scale = imcxx::Combo("U Scale"_j, conf.scale.U, scale_opt);
            auto v_scale = imcxx::Combo("V Scale"_j, conf.scale.V, scale_opt);

            AUTO_PROP(indirectStages[i].scale.U, u_scale);
            AUTO_PROP(indirectStages[i].scale.V, v_scale);
          }
          ImGui::EndTabItem();
        }
      }
      ImGui::EndTabBar();
    }

    // TODO: Multi selection
    for (int i = 0; i < stages.size(); ++i) {
      if (!opened[i]) {
        stages.erase(i);
        delegate.commit("Removed Indirect Configuration");
        break;
      }
    }
  }
  {
    auto& matrices = matData.mIndMatrices;
    std::array<bool, 3> opened{};
    if (ImGui::Button("Add a Matrix")) {
      matrices.push_back({});
      delegate.commit("Added Indirect Configuration");
    }

    if (ImGui::BeginTabBar("Matrices",
                           ImGuiTabBarFlags_AutoSelectNewTabs |
                               ImGuiTabBarFlags_FittingPolicyResizeDown)) {
      for (int i = 0; i < matrices.size(); ++i) {
        opened[i] = true;
        auto title = std::string("Matrix ") + std::to_string(i);
        if (ImGui::BeginTabItem(title.c_str(), &opened[i])) {
          auto* tm = &matrices[i];

          auto s = tm->scale;
          const auto rotate = glm::degrees(tm->rotate);
          auto r = rotate;
          auto t = tm->trans;
          ImGui::SliderFloat2("Scale", &s.x, 0.0f, 10.0f);
          ImGui::SliderFloat("Rotate", &r, 0.0f, 360.0f);
          ImGui::SliderFloat2("Translate", &t.x, -10.0f, 10.0f);
          AUTO_PROP(mIndMatrices[i].scale, s);
          if (r != rotate)
            AUTO_PROP(mIndMatrices[i].rotate, glm::radians(r));
          AUTO_PROP(mIndMatrices[i].trans, t);

          ImGui::EndTabItem();
        }
      }
      ImGui::EndTabBar();
    }

    // TODO: Multi selection
    for (int i = 0; i < matrices.size(); ++i) {
      if (!opened[i]) {
        matrices.erase(i);
        delegate.commit("Removed Indirect Configuration");
        break;
      }
    }
  }
}

kpi::RegisterPropertyView<IGCMaterial, IndirectSurface>
    IndirectSurfaceInstaller;

} // namespace libcube::UI
