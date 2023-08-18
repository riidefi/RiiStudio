#pragma once

#include "ErrorState.hpp"

#include <LibBadUIFramework/ActionMenu.hpp>
#include <core/common.h>
#include <frontend/PresetHelper.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <plugins/g3d/model.hpp>
#include <rsl/Defer.hpp>

namespace libcube::UI {

struct MergeAction {
  riistudio::g3d::Model* target_model;
  std::vector<librii::crate::CrateAnimation> source;
  // target_selection.size() == target_model->getMaterials().size()
  std::vector<std::optional<size_t>> source_selection;

  [[nodiscard]] Result<void>
  Populate(riistudio::g3d::Model* target_model_,
           std::vector<librii::crate::CrateAnimation>&& source_) {
    target_model = target_model_;
    source = std::move(source_);
    source_selection.clear();
    source_selection.resize(target_model->getMaterials().size());
    return RestoreDefaults();
  }

  [[nodiscard]] Result<void> MergeNothing() {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    std::ranges::fill(source_selection, std::nullopt);
    return {};
  }

  [[nodiscard]] Result<void> RestoreDefaults() {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    std::unordered_map<std::string, size_t> lut;
    for (auto&& [idx, preset] : rsl::enumerate(source)) {
      lut[preset.mat.name] = idx;
    }
    // TODO: rsl::ToList() is a hack
    for (auto&& [target_idx, target] :
         rsl::enumerate(target_model->getMaterials() | rsl::ToList())) {
      auto it = lut.find(target.name);
      if (it != lut.end()) {
        size_t source_idx = it->second;
        EXPECT(source_idx < source.size());
        source_selection[target_idx] = source_idx;
      } else {
        source_selection[target_idx] = std::nullopt;
      }
    }
    return {};
  }

  [[nodiscard]] Result<void>
  SetMergeSelection(size_t target_index, std::optional<size_t> source_index) {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    EXPECT(target_index < target_model->getMaterials().size());
    if (source_index.has_value()) {
      EXPECT(*source_index < source.size());
      source_selection[target_index] = *source_index;
    } else {
      source_selection[target_index] = std::nullopt;
    }
    return {};
  }

  [[nodiscard]] Result<void> DontMerge(size_t target_index) {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    EXPECT(target_index < target_model->getMaterials().size());
    source_selection[target_index] = std::nullopt;
    return {};
  }

  [[nodiscard]] Result<void> RestoreDefault(size_t target_index) {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    EXPECT(target_index < target_model->getMaterials().size());
    auto name = target_model->getMaterials()[target_index].name;
    // Since later entries in LUT will override previous ones, we need to
    // iterate backwards
    std::optional<size_t> source_index = std::nullopt;
    // TODO: rsl::ToList() is a hack
    std::vector<std::tuple<size_t, librii::crate::CrateAnimation>> _tmp =
        rsl::enumerate(source) | rsl::ToList();
    for (auto& [idx, source] : std::views::reverse(_tmp)) {
      if (source.mat.name == name) {
        source_index = idx;
        break;
      }
    }
    source_selection[target_index] = source_index;
    return {};
  }

  [[nodiscard]] Result<std::vector<std::string>> PerformMerge() {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    std::vector<std::string> logs;
    for (size_t i = 0; i < target_model->getMaterials().size(); ++i) {
      auto source_index = source_selection[i];
      if (!source_index.has_value()) {
        continue;
      }
      auto& target_mat = target_model->getMaterials()[i];
      auto& source_mat = source[*source_index];
      auto ok =
          riistudio::g3d::ApplyCratePresetToMaterial(target_mat, source_mat);
      if (!ok) {
        logs.push_back(
            std::format("Attempted to merge {} into {}. Failed with error: {}",
                        source_mat.mat.name, target_mat.name, ok.error()));
      }
    }
    return logs;
  }
};

void DrawComboSourceMat(MergeAction& action, int& item);
void DrawComboSourceMat(MergeAction& action, std::optional<size_t>& item);

struct MergeUIResult {
  struct None {};
  struct OK {
    std::vector<std::string> logs;
  };
  struct Cancel {};
  enum {
    None_i,
    OK_i,
    Cancel_i,
  };
};
using MergeUIResult_t =
    std::variant<MergeUIResult::None, MergeUIResult::OK, MergeUIResult::Cancel>;

[[nodiscard]] Result<MergeUIResult_t> DrawMergeActionUI(MergeAction& action);

class ImportPresetsAction
    : public kpi::ActionMenu<riistudio::g3d::Model, ImportPresetsAction> {
  struct State {
    struct None {};
    struct ImportDialog {};
    struct MergeDialog {
      MergeAction m_action;
    };
    struct Error : public ErrorState {
      Error(std::string&& err) : ErrorState("Preset Import Error") {
        ErrorState::enter(std::move(err));
      }
    };
  };
  std::variant<State::None, State::ImportDialog, State::MergeDialog,
               State::Error>
      m_state{State::None{}};

public:
  bool _context(riistudio::g3d::Model&) {
    if (ImGui::MenuItem("Import multiple presets"_j)) {
      m_state = State::ImportDialog{};
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Model& mdl) {
    if (m_state.index() == 3) {
      State::Error& error = *std::get_if<State::Error>(&m_state);
      error.enter(std::move(error.mError));
      error.modal();
      if (error.mError.empty()) {
        m_state = State::None{};
      }
      return kpi::NO_CHANGE;
    }

    if (m_state.index() == 1) {
      auto presets = rs::preset_helper::tryImportManyRsPreset();
      if (!presets) {
        m_state = State::Error{std::move(presets.error())};
        return kpi::NO_CHANGE;
      }
      MergeAction action;
      auto ok = action.Populate(&mdl, std::move(*presets));
      if (!ok) {
        m_state = State::Error{std::move(presets.error())};
        return kpi::NO_CHANGE;
      }
      m_state = State::MergeDialog{std::move(action)};
    }
    if (m_state.index() == 2) {
      auto& merge = *std::get_if<State::MergeDialog>(&m_state);
      ImGui::OpenPopup("Merge");
      ImGui::SetNextWindowSize(ImVec2{800.0f, 446.0f});
      if (ImGui::BeginPopup("Merge")) {
        RSL_DEFER(ImGui::EndPopup());
        auto ok = DrawMergeActionUI(merge.m_action);
        if (!ok) {
          m_state = State::Error{std::move(ok.error())};
          return kpi::NO_CHANGE;
        }
        switch (ok->index()) {
        case MergeUIResult::OK_i: {
          auto logs = std::move(std::get_if<MergeUIResult::OK>(&*ok)->logs);
          if (logs.size()) {
            auto err = std::format("The following errors were encountered:\n{}",
                                   rsl::join(logs, "\n"));
            m_state = State::Error{std::move(err)};
            return kpi::CHANGE;
          } else {
            m_state = State::None{};
            return kpi::CHANGE_NEED_RESET;
          }
        }
        case MergeUIResult::Cancel_i:
          m_state = State::None{};
          return kpi::NO_CHANGE;
        case MergeUIResult::None_i:
          return kpi::NO_CHANGE;
        }
      }
    }

    return kpi::NO_CHANGE;
  }
};

} // namespace libcube::UI
