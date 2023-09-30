#include <frontend/widgets/ReflectedPropertyGrid.hpp>
#include "frontend/widgets/OutlinerWidget.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "JpaEditor.hpp"

#include "frontend/widgets/Lib3dImage.hpp"

namespace riistudio::frontend {

void JpaEditorPropertyGrid::Draw(librii::jpa::JPADynamicsBlock* block) {

  block->volumeType =
      imcxx::EnumCombo("Emitter Volume Type", block->volumeType);

  ImGui::InputFloat3("Emitter Scale", value_ptr(block->emitterScl));
  ImGui::InputFloat3("Emitter Rotation", value_ptr(block->emitterRot));
  ImGui::InputFloat3("Emitter Translation", value_ptr(block->emitterTrs));
  ImGui::InputFloat3("Emitter Direction", value_ptr(block->emitterDir));

  ImGui::InputFloat("Initial Velocity Omni", &block->initialVelOmni);
  ImGui::InputFloat("Initial Velocity Axis", &block->initialVelAxis);
  ImGui::InputFloat("Initial Velocity Random", &block->initialVelRndm);
  ImGui::InputFloat("Initial Velocity Direction", &block->initialVelDir);
  ImGui::InputFloat("Spread", &block->spread);
  ImGui::InputFloat("Initial Velocity Ratio", &block->initialVelRatio);

  ImGui::InputFloat("Rate", &block->rate);
  ImGui::InputFloat("Rate Random", &block->rateRndm);
  ImGui::InputScalar("Rate Step", ImGuiDataType_U8, &block->rateStep);

  // There is no wrapper for a U16 input
  ImGui::InputScalar("LifeTime", ImGuiDataType_U16, &block->lifeTime);

  ImGui::InputFloat("LifeTime Random", &block->lifeTimeRndm);

  ImGui::InputScalar("Volume Size", ImGuiDataType_U16, &block->volumeSize);
  ImGui::InputFloat("Volume Sweep", &block->volumeSweep);
  ImGui::InputFloat("Volume Minimum Radius", &block->volumeMinRad);

  ImGui::InputFloat("Air Resistance", &block->airResist);
  ImGui::InputFloat("Air Resistance Random", &block->airResistRndm);

  ImGui::InputFloat("Moment", &block->moment);
  ImGui::InputFloat("Moment Random", &block->momentRndm);

  ImGui::InputFloat("Acceleration", &block->accel);
  ImGui::InputFloat("Acceleration Random", &block->accelRndm);

  ImGui::InputScalar("Max Frame", ImGuiDataType_U16, &block->maxFrame);
  ImGui::InputScalar("Start Frame", ImGuiDataType_U16, &block->startFrame);

  ImGui::InputScalar("Div Number", ImGuiDataType_U16, &block->divNumber);

}

void JpaEditorPropertyGrid::Draw(librii::jpa::JPABaseShapeBlock* block) {
  block->shapeType = imcxx::EnumCombo("Shape Type", block->shapeType);
  block->dirType = imcxx::EnumCombo("Direction Type", block->dirType);
  block->rotType = imcxx::EnumCombo("Rotation Type", block->rotType);

  ImGui::InputFloat2("Base Size", glm::value_ptr(block->baseSize));

  ImGui::SliderFloat("Tiling S", &block->tilingS, 0, 10, "%.3f", 0);
  ImGui::SliderFloat("Tiling T", &block->tilingT, 0, 10, "%.3f", 0);

  ImGui::Checkbox("Draw Forward Ahead", &block->isDrawFwdAhead);
  ImGui::Checkbox("Draw Parent Ahead", &block->isDrawPrntAhead);
  ImGui::Checkbox("No Draw Parent", &block->isNoDrawParent);
  ImGui::Checkbox("No Draw Child", &block->isNoDrawChild);

  ImGui::Text("Texture Palette Animation");

  ImGui::Checkbox("Enabled", &block->isGlblTexAnm);
  block->texCalcIdxType =
      imcxx::EnumCombo("Calculation Index Type", block->texCalcIdxType);
  ImGui::InputScalar("Texture Index", ImGuiDataType_U8, &block->texIdx);

  if (ImGui::TreeNodeEx("Texture Anim List", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (int i = 0; i < block->texIdxAnimData.size(); i++) {

      ImGui::PushItemWidth(-1);
      auto str = std::format("texture Id {}", i);
      ImGui::PushID(str.c_str());
      ImGui::InputScalar("Rate Step", ImGuiDataType_U8,
                         &block->texIdxAnimData[i]);
      ImGui::PopID();
      ImGui::PopItemWidth();
    }
    ImGui::TreePop();
  }

  ImGui::InputScalar("Texture Index Loop Offset Mask", ImGuiDataType_U8,
                     &block->texIdxLoopOfstMask);

  ImGui::Text("Texture Coordinate Animation");

  ImGui::PushID("Texture Coordinate Animation");
  ImGui::Checkbox("Enabled", &block->isEnableProjection);
  ImGui::PopID();


  ImGui::Checkbox("Texture Scroll Animation", &block->isEnableTexScrollAnm);

  ImGui::SliderFloat("Texture Initial Translation X", &block->texInitTransX, 0,
                     10, "%.3f", 0);
  ImGui::SliderFloat("Texture Initial Translation Y", &block->texInitTransY, 0,
                     10, "%.3f", 0);
  ImGui::SliderFloat("Texture Initial Scale X", &block->texInitScaleX, 0, 10,
                     "%.3f", 0);
  ImGui::SliderFloat("Texture Initial Scale Y", &block->texInitScaleY, 0, 10,
                     "%.3f", 0);

  ImGui::SliderFloat("Texture Initial Rotation", &block->texInitRot, 0, 1,
                     "%.3f", 0);
  ImGui::SliderFloat("Texture Rotation Increment", &block->texIncRot, 0, 1,
                     "%.3f", 0);
  ImGui::SliderFloat("Texture Translation X Increment",
                     &block->texIncTransX, 0,
                     10, "%.3f", 0);
  ImGui::SliderFloat("Texture Translation Y Increment",
                     &block->texIncTransY, 0,
                     10, "%.3f", 0);

  ImGui::SliderFloat("Texture Scale X Increment", &block->texIncScaleX, 0, 0.1f,
                     "%.3f", 0);
  ImGui::SliderFloat("Texture Scale Y Increment", &block->texIncScaleY, 0, 0.1f,
                     "%.3f", 0);

  // Color Animation Settings
  ImGui::Text("Color Animation Settings");

  ImGui::PushID("Color Animation Settings");
  ImGui::Checkbox("Enabled", &block->isGlblClrAnm);
  ImGui::PopID();


  block->colorCalcIdxType =
      imcxx::EnumCombo("Color Calcuation Index", block->colorCalcIdxType);

  ImGui::ColorEdit4("Primary Color", block->colorPrm);
  ImGui::ColorEdit4("Environment Color", block->colorEnv);

  // We want to scale the gradients
  if(ImGui::InputScalar("Max Color Animation Frame", ImGuiDataType_U16,
                     &block->colorAnimMaxFrm)) {
    refreshGradientWidgets(block);
  }
  ImGui::InputScalar("Color Loop Offset Mask", ImGuiDataType_U8,
                     &block->colorLoopOfstMask);

  colorPrmGradient.widget("Color Primary Animation Gradient");
  // After drawing the gradient we must update the underlying color table
  // entries

  block->colorPrmAnimData.clear();

  // Because the marks are stored in a list internally
  auto markPrm = colorPrmGradient.gradient().get_marks().begin();
  for (int i = 0; i < colorPrmGradient.gradient().get_marks().size(); i++) {

    librii::gx::ColorF32 markColor = librii::gx::ColorF32();

    markColor.r = markPrm->color.x;
    markColor.g = markPrm->color.y;
    markColor.b = markPrm->color.z;
    markColor.a = markPrm->color.w;

    block->colorPrmAnimData.push_back(librii::jpa::ColorTableEntry(
        markPrm->position.get() * block->colorAnimMaxFrm, markColor));

    std::advance(markPrm,1);
  }



    colorEnvGradient.widget("Color Env Animation Gradient");
    // After drawing the gradient we must update the underlying color table entries

    block->colorEnvAnimData.clear();

    // Because the marks are stored in a list internally 
    auto mark = colorEnvGradient.gradient().get_marks().begin();
    for (int i = 0; i < colorEnvGradient.gradient().get_marks().size(); i++) {

      librii::gx::ColorF32 markColor = librii::gx::ColorF32();

      markColor.r = mark->color.x;
      markColor.g = mark->color.y;
      markColor.b = mark->color.z;
      markColor.a = mark->color.w;

      block->colorEnvAnimData.push_back(librii::jpa::ColorTableEntry(
          mark->position.get() * block->colorAnimMaxFrm, markColor));

      std::advance(mark, 1);
    }

}


void JpaEditorPropertyGrid::Draw(librii::jpa::JPAExtraShapeBlock* block) {
  ImGui::Checkbox("Enable Scale", &block->isEnableScale);
  ImGui::Checkbox("Is Diff XY", &block->isDiffXY);
  ImGui::Checkbox("Enable Scale By Speed X", &block->isEnableScaleBySpeedX);
  ImGui::Checkbox("Enable Scale By Speed Y", &block->isEnableScaleBySpeedY);

  block->scaleAnmTypeX = imcxx::EnumCombo("Scale Animation Type X",
                                          block->scaleAnmTypeX);
  block->scaleAnmTypeY =
      imcxx::EnumCombo("Scale Animation Type Y", block->scaleAnmTypeY);

  ImGui::SliderFloat("Scale In Timing", &block->scaleInTiming, 0, 10, "%.3f",
                     0);
  ImGui::SliderFloat("Scale Out Timing", &block->scaleOutTiming, 0, 10, "%.3f",
                     0);
  ImGui::SliderFloat("Scale In Value X", &block->scaleInValueX, 0, 10, "%.3f",
                     0);
  ImGui::SliderFloat("Scale In Value Y", &block->scaleInValueY, 0, 10, "%.3f",
                     0);
  ImGui::SliderFloat("Scale Out Value X", &block->scaleOutValueX, 0, 10, "%.3f",
                     0);
  ImGui::SliderFloat("Scale Out Value Y", &block->scaleOutValueY, 0, 10, "%.3f",
                     0);
  ImGui::SliderFloat("Scale Out Random", &block->scaleOutRandom, 0, 10, "%.3f",
                     0);
  ImGui::InputScalar("Scale Anm Max Frame X", ImGuiDataType_U16,
                     &block->scaleAnmMaxFrameX);
  ImGui::InputScalar("Scale Anm Max Frame Y", ImGuiDataType_U16,
                     &block->scaleAnmMaxFrameY);
  ImGui::InputFloat("Scale Increase Rate X", &block->scaleIncreaseRateX);
  ImGui::InputFloat("Scale Increase Rate Y", &block->scaleIncreaseRateY);

  ImGui::InputFloat("Scale Decrease Rate X", &block->scaleDecreaseRateX);
  ImGui::InputFloat("Scale Decrease Rate Y", &block->scaleDecreaseRateY);

  ImGui::Checkbox("Enable Rotate", &block->isEnableRotate);

  ImGui::InputScalar("Pivot X", ImGuiDataType_U8, &block->pivotX);
  ImGui::InputScalar("Pivot Y", ImGuiDataType_U8, &block->pivotY);

  ImGui::InputFloat("Angle", &block->rotateAngle);
  ImGui::InputFloat("Angle random", &block->rotateAngleRandom);

  ImGui::InputFloat("Speed", &block->rotateSpeed);
  ImGui::InputFloat("Speed random", &block->rotateSpeedRandom);
  ImGui::InputFloat("Direction", &block->rotateSpeedRandom);

  ImGui::Checkbox("Enable Alpha", &block->isEnableAlpha);
  ImGui::Checkbox("Enable Sine Wave", &block->isEnableSinWave);

  if (block->isEnableSinWave) {
    block->alphaWaveType =
        imcxx::EnumCombo("Alpha Wave Type", block->alphaWaveType);
  }

  ImGui::InputFloat("In Timing", &block->alphaInTiming);
  ImGui::InputFloat("Out Timing", &block->alphaOutTiming);

  ImGui::InputFloat("Base Value", &block->alphaBaseValue);
  ImGui::InputFloat("In Value", &block->alphaInValue);
  ImGui::InputFloat("Out Value", &block->alphaOutValue);

  ImGui::InputFloat("Increase Rate", &block->alphaIncreaseRate);
  ImGui::InputFloat("Decrease Rate", &block->alphaDecreaseRate);

  ImGui::InputFloat("Param 1", &block->alphaWaveParam1);
  ImGui::InputFloat("Param 2", &block->alphaWaveParam2);
  ImGui::InputFloat("Param 3", &block->alphaWaveParam3);

  ImGui::InputFloat("Wave Random", &block->alphaWaveRandom);

}

void JpaEditorPropertyGrid::Draw(librii::jpa::JPAFieldBlock* block) {
  block->type = imcxx::EnumCombo("Field Type", block->type);
  block->addType = imcxx::EnumCombo("Field Add Type", block->addType);
  // block.addType = imcxx::EnumCombo("Field Status Type", block.addType);

  ImGui::InputFloat("Max Distance", &block->maxDist);

  ImGui::InputFloat3("Field Position", value_ptr(block->pos));
  ImGui::InputFloat3("Field Direction", value_ptr(block->dir));

  ImGui::InputFloat("Fade In", &block->fadeIn);
  ImGui::InputFloat("Fade Out", &block->fadeOut);

  ImGui::InputFloat("disTime", &block->disTime);
  ImGui::InputFloat("enTime", &block->enTime);
  ImGui::InputScalar("Cycle", ImGuiDataType_U8, &block->cycle);
  ImGui::InputFloat("fade In Rate", &block->fadeInRate);
  ImGui::InputFloat("fade Out Rate", &block->fadeOutRate);

  ImGui::InputFloat("Magnitude", &block->mag);

  if (block->type == librii::jpa::FieldType::Drag) {
    ImGui::InputFloat("Magnitude Random", &block->magRndm);
  }
  if (block->type == librii::jpa::FieldType::Newton ||
      block->type == librii::jpa::FieldType::Air ||
      block->type == librii::jpa::FieldType::Convection) {
    ImGui::InputFloat("Reference Distance", &block->refDistance);
  }
  if (block->type == librii::jpa::FieldType::Vortex ||
      block->type == librii::jpa::FieldType::Spin) {
    ImGui::InputFloat("Inner Speed", &block->innerSpeed);
  }
  if (block->type == librii::jpa::FieldType::Vortex) {
    ImGui::InputFloat("Outer Speed", &block->outerSpeed);
  }
}

void JpaEditorPropertyGrid::Draw(librii::jpa::JPAExTexBlock* block) {
  ImGui::InputScalar("Indirect Texture ID", ImGuiDataType_U8, &block->indTextureID);
  ImGui::InputScalar("Sub Texture ID", ImGuiDataType_U8, &block->subTextureID);

  ImGui::InputScalar("Second Texture ID", ImGuiDataType_U8, &block->secondTextureIndex);
}


 void JpaEditorPropertyGrid::refreshGradientWidgets(
    librii::jpa::JPABaseShapeBlock* block) {
  colorEnvGradient.gradient().clear();
  colorPrmGradient.gradient().clear();


  for (auto& color_entry : block->colorPrmAnimData) {
    f32 position = (float)color_entry.timeBegin / (float)block->colorAnimMaxFrm;

    colorPrmGradient.gradient().add_mark(
        ImGG::Mark{ImGG::RelativePosition{position},
                   ImVec4{color_entry.color.r, color_entry.color.g,
                          color_entry.color.b, color_entry.color.a}});
  }


  for (auto &color_entry: block->colorEnvAnimData) {
    f32 position = (float)color_entry.timeBegin / (float)block->colorAnimMaxFrm;

    colorEnvGradient.gradient().add_mark(
        ImGG::Mark{
                           ImGG::RelativePosition{position},
        ImVec4{color_entry.color.r, color_entry.color.g, color_entry.color.b,
               color_entry.color.a}});
  }

}


void JpaEditorPropertyGrid::Draw(librii::jpa::TextureBlock block) {
  preview.draw(block);
}


} // namespace riistudio::frontend
