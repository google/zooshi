// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "game.h"
#include "states/pause_state.h"

#include "flatui/flatui.h"
#include "flatui/flatui_common.h"
#include "fplbase/input.h"
#include "states/states_common.h"
#include "world.h"

namespace fpl {
namespace fpl_project {

using gui::TextButton;

static const auto kPauseStateLabelSize = 150.0f;
static const auto kPauseStateButtonSize = 100.0f;

void PauseState::Initialize(InputSystem* input_system, World* world,
                            const Config* config, AssetManager* asset_manager,
                            FontManager* font_manager,
                            pindrop::AudioEngine* audio_engine) {
  asset_manager_ = asset_manager;
  font_manager_ = font_manager;
  input_system_ = input_system;
  world_ = world;
  audio_engine_ = audio_engine;

  sound_continue_ = audio_engine->GetSoundHandle("continue");
  sound_exit_ = audio_engine->GetSoundHandle("exit");

  background_paused_ =
      asset_manager_->LoadTexture("textures/ui_background_base.webp");

#ifdef ANDROID_CARDBOARD
  cardboard_camera_.set_viewport_angle(config->cardboard_viewport_angle());
#else
  (void)config;
#endif
}

void PauseState::AdvanceFrame(int /*delta_time*/, int* next_state) {
  UpdateMainCamera(&main_camera_, world_);

  *next_state = next_state_;

  // Unpause
  if (input_system_->GetButton(FPLK_p).went_down()) {
    *next_state = kGameStateGameplay;
  }

  // Exit the game.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(FPLK_AC_BACK).went_down()) {
    *next_state = kGameStateGameMenu;
  }

  if (*next_state == kGameStateGameplay) {
    audio_engine_->PlaySound(sound_continue_);
  } else if (*next_state == kGameStateGameMenu) {
    audio_engine_->PlaySound(sound_exit_);
  }

  next_state_ = kGameStatePause;
}

GameState PauseState::PauseMenu(AssetManager& assetman, FontManager& fontman,
                                InputSystem& input) {
  GameState next_state = kGameStatePause;

  gui::Run(assetman, fontman, input, [&]() {
    gui::StartGroup(gui::kLayoutHorizontalTop, 0);

    // Background image.
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    // Positioning the UI slightly above of the center.
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(0, -150));
    gui::Image(*background_paused_, 850);
    gui::EndGroup();

    // Menu items. Note that we are layering 2 layouts here
    // (background + menu items).
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(0, -150));
    gui::SetMargin(gui::Margin(200, 280, 200, 100));
    gui::StartGroup(gui::kLayoutVerticalLeft, 50, "menu");
    gui::SetMargin(gui::Margin(10));
    gui::SetTextColor(kColorBrown);
    gui::Label("Paused", kPauseStateLabelSize);
    gui::EndGroup();

    auto event = TextButton("Continue", kPauseStateButtonSize, gui::Margin(2));
    if (event & gui::kEventWentUp) {
      next_state = kGameStateGameplay;
    }

    event =
        TextButton("Return to Title", kPauseStateButtonSize, gui::Margin(2));
    if (event & gui::kEventWentUp) {
      next_state = kGameStateGameMenu;
    }
    gui::EndGroup();
    gui::EndGroup();
  });

  return next_state;
}

void PauseState::RenderPrep(Renderer* renderer) {
  world_->world_renderer->RenderPrep(main_camera_, *renderer, world_);
}

void PauseState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_CARDBOARD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
}

void PauseState::HandleUI(Renderer* renderer) {
  // No culling when drawing the menu.
  renderer->SetCulling(Renderer::kNoCulling);
  next_state_ = PauseMenu(*asset_manager_, *font_manager_, *input_system_);
}

void PauseState::OnEnter(int /*previous_state*/) {
  world_->player_component.set_active(false);
  input_system_->SetRelativeMouseMode(false);
}

}  // fpl_project
}  // fpl
