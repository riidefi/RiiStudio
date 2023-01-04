#include <core/kpi/Plugins.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/kpi/RichNameManager.hpp>
#include <imcxx/Widgets.hpp>
#include <llvm/Support/Casting.h>
#include <plugins/gc/Export/Material.hpp>
#include <plugins/mk/BFG/Fog.hpp>
#include <plugins/mk/BFG/io/BFG.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::mk {

using namespace llvm;

namespace ui {

auto fogEntry =
    kpi::StatelessPropertyView<FogEntry>()
        .setTitle("Properties")
        .setIcon((const char*)ICON_FA_COGS)
        .onDraw([](kpi::PropertyDelegate<FogEntry>& delegate) {
          auto& fog = delegate.getActive();
          int orthoOrPersp =
              static_cast<int>(fog.mType) >=
                      static_cast<int>(librii::gx::FogType::OrthographicLinear)
                  ? 0
                  : 1;
          int fogType = static_cast<int>(fog.mType);
          if (fogType >=
              static_cast<int>(librii::gx::FogType::OrthographicLinear))
            fogType -= 5;
          ImGui::Combo("Projection", &orthoOrPersp,
                       "Orthographic\0Perspective\0");
          ImGui::Combo("Function", &fogType,
                       "None\0Linear\0Exponential\0Quadratic\0Inverse "
                       "Exponential\0Quadratic\0");
          int new_type = fogType;
          if (new_type != 0)
            new_type += (1 - orthoOrPersp) * 5;
          KPI_PROPERTY_EX(delegate, mType,
                          static_cast<librii::gx::FogType>(new_type));

          bool enabled = fog.mEnabled;
          ImGui::Checkbox("Fog Enabled", &enabled);
          KPI_PROPERTY_EX(delegate, mEnabled, static_cast<u16>(enabled));
          {
            util::ConditionalActive g(enabled);
            float startZ = fog.mStartZ;
            ImGui::InputFloat("Start Z", &startZ);
            KPI_PROPERTY_EX(delegate, mStartZ, startZ);

            float endZ = fog.mEndZ;
            ImGui::InputFloat("End Z", &endZ);
            KPI_PROPERTY_EX(delegate, mEndZ, endZ);

            librii::gx::ColorF32 clr_f32 = fog.mColor;
            ImGui::ColorEdit4("Fog Color", clr_f32);
            KPI_PROPERTY_EX(delegate, mColor, (librii::gx::Color)clr_f32);

            int center = fog.mCenter;
            ImGui::InputInt("Center", &center);
            KPI_PROPERTY_EX(delegate, mCenter, static_cast<u16>(center));

            float fadeSpeed = fog.mFadeSpeed;
            ImGui::SliderFloat("Fade Speed", &fadeSpeed, 0.0f, 1.0f);
            KPI_PROPERTY_EX(delegate, mFadeSpeed, fadeSpeed);
          }
        });
} // namespace ui
kpi::DecentralizedInstaller
    TypeInstaller([](kpi::ApplicationPlugins& installer) {
      installer.addType<BinaryFog>();
      kpi::RichNameManager::getInstance().addRichName<mk::FogEntry>(
          (const char*)ICON_FA_CLOUD, "Fog Entry", (const char*)ICON_FA_CLOUD,
          "Fog Entries");

      installer.addDeserializer<BFG>();
      installer.addSerializer<BFG>();
    });
} // namespace riistudio::mk
