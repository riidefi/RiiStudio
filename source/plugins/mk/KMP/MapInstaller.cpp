#include <core/kpi/Plugins.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/kpi/RichNameManager.hpp>
#include <core/util/gui.hpp>
#include <llvm/Support/Casting.h>
#include <plugins/mk/KMP/Map.hpp>
#include <plugins/mk/KMP/io/KMP.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::mk {

using namespace llvm;

template <typename ObjectT> struct DelegateWrapper {
  DelegateWrapper(kpi::PropertyDelegate<ObjectT>& delegate)
      : mDelegate(delegate) {}

  template <typename T, typename... Args>
  void DragFloat3(const char* title, T prop, Args... args) {
    auto trans = mDelegate.getActive().*prop;
    ImGui::DragFloat3(title, &trans.x, &args...);
    mDelegate.propertyEx(prop, trans);
  }
  template <typename T, typename... Args>
  void SliderFloat3(const char* title, T prop, Args... args) {
    auto trans = mDelegate.getActive().*prop;
    ImGui::SliderFloat3(title, &trans.x, args...);
    mDelegate.propertyEx(prop, trans);
  }
  template <typename T, typename... Args>
  void InputInt(const char* title, T prop, Args... args) {
    auto raw = mDelegate.getActive().*prop;
    int trans = raw;
    ImGui::InputInt(title, &trans, args...);
    raw = trans;
    mDelegate.propertyEx(prop, raw);
  }
  template <typename T, typename U, typename... Args>
  void InputIntA(const char* title, T get, U set, Args... args) {
    auto raw = (mDelegate.getActive().*get)();
    int trans = raw;
    ImGui::InputInt(title, &trans, args...);
    raw = trans;
    mDelegate.propertyAbstract(get, set, raw);
  }
  template <typename T, typename U, typename... Args>
  void SliderIntA(const char* title, T get, U set, Args... args) {
    auto raw = (mDelegate.getActive().*get)();
    int trans = raw;
    ImGui::SliderInt(title, &trans, args...);
    raw = trans;
    mDelegate.propertyAbstract(get, set, raw);
  }

  kpi::PropertyDelegate<ObjectT>& mDelegate;
};
namespace ui {
auto ktpt = kpi::StatelessPropertyView<StartPoint>()
                .setTitle("Properties")
                .setIcon((const char*)ICON_FA_COGS)
                .onDraw([](kpi::PropertyDelegate<StartPoint>& delegate) {
                  DelegateWrapper w(delegate);

                  w.DragFloat3("Position", &StartPoint::position);
                  w.SliderFloat3("Rotation", &StartPoint::rotation, 0, 360);
                  w.InputInt("Player Index", &StartPoint::player_index);
                  w.InputInt("Padding", &StartPoint::_);
                });

auto area =
    kpi::StatelessPropertyView<Area>()
        .setTitle("Properties")
        .setIcon((const char*)ICON_FA_COGS)
        .onDraw([](kpi::PropertyDelegate<Area>& delegate) {
          DelegateWrapper w(delegate);

          w.SliderIntA("Priority", &Area::getPriority, &Area::setPriority, 0,
                       0xff);

          // TODO -- Spillover of settings of different types.
          Area* pArea = &delegate.getActive();

          auto draw_area = [&] {
            if (auto* camArea = dyn_cast<CameraArea>(pArea);
                camArea != nullptr) {
              ImGui::Text("Camera Area");
              return;
            }

            if (auto* efArea = dyn_cast<EffectArea>(pArea); efArea != nullptr) {
              ImGui::Text("Effect Area");
              return;
            }

            if (auto* fogArea = dyn_cast<FogArea>(pArea); fogArea != nullptr) {
              ImGui::Text("Fog Area");
              return;
            }

            if (auto* pullArea = dyn_cast<PullArea>(pArea);
                pullArea != nullptr) {
              ImGui::Text("Pull Area");
              return;
            }

            if (auto* enemyFallArea = dyn_cast<EnemyFallArea>(pArea);
                enemyFallArea != nullptr) {
              ImGui::Text("EnemyFall Area");
              return;
            }

            if (auto* map2DArea = dyn_cast<Map2DArea>(pArea);
                map2DArea != nullptr) {
              ImGui::Text("Map2D Area");
              return;
            }

            if (auto* soundArea = dyn_cast<SoundArea>(pArea);
                soundArea != nullptr) {
              ImGui::Text("Sound Area");
              return;
            }

            if (auto* objClipArea = dyn_cast<ObjClipArea>(pArea);
                objClipArea != nullptr) {
              ImGui::Text("ObjClip Area");
              ImGui::Text(isa<ObjClipClassifierArea>(pArea) ? "Classifier"
                                                            : "Discriminator");
              return;
            }

            if (auto* boundaryArea = dyn_cast<BoundaryArea>(pArea);
                boundaryArea != nullptr) {
              ImGui::Text("Boundary Area");

              bool constrained = boundaryArea->isConstrained();
              ImGui::Checkbox("Constrained", &constrained);
              if (constrained && !boundaryArea->isConstrained())
                boundaryArea->defaultConstraint();
              else if (!constrained && boundaryArea->isConstrained())
                boundaryArea->forgetConstraint();

              ImGui::Indent(20);
              {
                riistudio::util::ConditionalActive g(constrained);

                int c = static_cast<int>(boundaryArea->getConstraintType());
                ImGui::Combo("Constaint", &c,
                             "Enable INSIDE the range.\0Enable OUTSIDE the "
                             "range.\0");
                if (constrained)
                  boundaryArea->setConstraintType(
                      static_cast<BoundaryArea::ConstraintType>(c));

                ImGui::PushItemWidth(50);
                {
                  int lo = boundaryArea->getInclusiveLowerBound();
                  int hi = boundaryArea->getInclusiveUpperBound();
                  ImGui::SliderInt("##Low", &lo, 0, 0xff);
                  ImGui::SameLine();

                  ImGui::Text(" <= Checkpoint ID <=");
                  ImGui::SameLine();

                  ImGui::SliderInt("##High", &hi, 0, 0xff);

                  if (hi >= lo && lo != hi + 1) {
                    boundaryArea->setInclusiveLowerBound(lo);
                    boundaryArea->setInclusiveUpperBound(hi);
                  }
                }
                ImGui::PopItemWidth();
              }
              ImGui::Indent(.0f);
              return;
            }
          };

          draw_area();
        });

auto gobj =
    kpi::StatelessPropertyView<GeoObj>()
        .setTitle("Properties")
        .setIcon((const char*)ICON_FA_COGS)
        .onDraw([](kpi::PropertyDelegate<GeoObj>& delegate) {
          DelegateWrapper w(delegate);

          w.InputInt("ID", &GeoObj::id);
          w.InputInt("Padding", &GeoObj::_);
          w.DragFloat3("Position", &GeoObj::position);
          w.SliderFloat3("Rotation", &GeoObj::rotation, 0, 360);
          w.DragFloat3("Scaling", &GeoObj::scale);
          w.InputInt("Rail Index", &GeoObj::pathId);
          w.InputInt("Flag", &GeoObj::flags);
          if (ImGui::BeginTable("Settings", 8, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow();
            for (int i = 0; i < 8; ++i) {
              ImGui::TableSetColumnIndex(i);
              int s = delegate.getActive().settings[i];
              ImGui::InputInt(std::to_string(i).c_str(), &s);
              KPI_PROPERTY_EX(delegate, settings[i], static_cast<u16>(s));
            }
            ImGui::EndTable();
          }
        });

auto ckph = kpi::StatelessPropertyView<CheckPath>()
                .setTitle("Properties")
                .setIcon((const char*)ICON_FA_COGS)
                .onDraw([](kpi::PropertyDelegate<CheckPath>& delegate) {
                  DelegateWrapper w(delegate);
                  auto& path = delegate.getActive();

                  // Intrusive directed graph
                  auto pred = path.mPredecessors;
                  auto succ = path.mSuccessors;
                  ImGui::Text("Predecessors");
                  for (auto p : pred)
                    ImGui::Text("%u", static_cast<unsigned int>(p));
                  ImGui::Text("Successors");
                  for (auto p : succ)
                    ImGui::Text("%u", static_cast<unsigned int>(p));

                  if (ImGui::BeginTable("Points", 4, ImGuiTableFlags_Borders)) {
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Left");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Right");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("Respawn");
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("LapCheck");
                    for (auto& pt : path.mPoints) {
                      ImGui::TableNextRow();
                      ImGui::TableSetColumnIndex(0);
                      auto left = pt.getLeft();
                      ImGui::InputFloat2("Left", &left.x);

                      ImGui::TableSetColumnIndex(1);
                      auto right = pt.getRight();
                      ImGui::InputFloat2("Right", &right.x);

                      ImGui::TableSetColumnIndex(2);
                      int respawn = pt.getRespawnIndex();
                      ImGui::InputInt("Respawn", &respawn);

                      ImGui::TableSetColumnIndex(3);
                      int lapcheck = pt.getLapCheck();
                      ImGui::InputInt("LapCheck", &lapcheck);
                    }
                    ImGui::EndTable();
                  }
                });

} // namespace ui
kpi::DecentralizedInstaller
    TypeInstaller([](kpi::ApplicationPlugins& installer) {
      installer.addType<CourseMap>();
      kpi::RichNameManager& rich = kpi::RichNameManager::getInstance();
      rich.addRichName<mk::StartPoint>((const char*)ICON_FA_PLAY,
                                       "Starting Point");
      rich.addRichName<mk::EnemyPath>((const char*)ICON_FA_ROBOT, "Enemy Path");
      rich.addRichName<mk::ItemPath>((const char*)ICON_FA_CUBE, "Item Path");
      rich.addRichName<mk::CheckPath>((const char*)ICON_FA_FLAG, "Check Path");
      rich.addRichName<mk::Rail>((const char*)ICON_FA_PROJECT_DIAGRAM, "Rail");
      rich.addRichName<mk::GeoObj>((const char*)ICON_FA_CUBE, "Object",
                                   (const char*)ICON_FA_CUBES);
      rich.addRichName<mk::Area>((const char*)ICON_FA_OBJECT_UNGROUP, "Area");
      rich.addRichName<mk::Camera>((const char*)ICON_FA_VIDEO, "Camera");
      rich.addRichName<mk::RespawnPoint>((const char*)ICON_FA_STEP_BACKWARD,
                                         "Respawn Point");
      rich.addRichName<mk::Cannon>((const char*)ICON_FA_ROCKET, "Cannon Point");
      rich.addRichName<mk::Stage>((const char*)ICON_FA_COGS, "Stage");
      rich.addRichName<mk::MissionPoint>((const char*)ICON_FA_CUBE,
                                         "Mission Point");

      installer.addDeserializer<KMP>();
      installer.addSerializer<KMP>();
    });

} // namespace riistudio::mk
