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

#include "states/pause_state.h"

#include "flatui/flatui.h"
#include "fplbase/input.h"
#include "states/states.h"
#include "states/states_common.h"
#include "world.h"

namespace fpl {
namespace fpl_project {

void PauseState::Initialize(InputSystem* input_system, World* world,
                            AssetManager* asset_manager,
                            FontManager* font_manager,
                            pindrop::AudioEngine* audio_engine) {
  asset_manager_ = asset_manager;
  font_manager_ = font_manager;
  input_system_ = input_system;
  world_ = world;
  audio_engine_ = audio_engine;

  sound_continue_ = audio_engine->GetSoundHandle("continue");
  sound_exit_ = audio_engine->GetSoundHandle("exit");
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

static GameState PauseMenu(AssetManager& assetman, FontManager& fontman,
                           InputSystem& input) {
  GameState next_state = kGameStatePause;
  gui::Run(assetman, fontman, input, [&]() {
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter, mathfu::kZeros2f);
    gui::Label("Paused", 120);
    gui::SetMargin(gui::Margin(30));
    auto event = TextButton("CONTINUE", 100, "button");
    if (event & gui::kEventWentUp) {
      next_state = kGameStateGameplay;
    }
    event = TextButton("EXIT", 100, "button");
    if (event & gui::kEventWentUp) {
      next_state = kGameStateGameMenu;
    }
    gui::EndGroup();
  });

  return next_state;
}

void PauseState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_CARDBOARD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
  next_state_ = PauseMenu(*asset_manager_, *font_manager_, *input_system_);
}

void PauseState::OnEnter() {
  world_->player_component.set_active(false);
  input_system_->SetRelativeMouseMode(false);
}

}  // fpl_project
}  // fpl
