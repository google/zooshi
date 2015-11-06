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

#include "states/game_over_state.h"

#include "components/attributes.h"
#include "components/sound.h"
#include "config_generated.h"
#include "fplbase/input.h"
#include "game.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "save_data_generated.h"
#include "states/game_menu_state.h"
#include "states/states.h"
#include "states/states_common.h"
#include "world.h"

namespace fpl {
namespace zooshi {

static const float kTimeToStopRaft = 500.0f;

void GameOverState::Initialize(InputSystem* input_system, World* world,
                               const Config* config,
                               AssetManager* asset_manager,
                               FontManager* font_manager,
                               GPGManager* gpg_manager,
                               pindrop::AudioEngine* audio_engine) {
  world_ = world;

  // Set references used in GUI.
  config_ = config;
  input_system_ = input_system;
  asset_manager_ = asset_manager;
  font_manager_ = font_manager;
  gpg_manager_ = gpg_manager;
  audio_engine_ = audio_engine;

  sound_click_ = audio_engine->GetSoundHandle("click");

  // Retrieve references to textures. (Loading process is done already.)
  background_game_over_ =
      asset_manager_->LoadTexture("textures/ui_background_base.webp");

#ifdef ANDROID_HMD
  cardboard_camera_.set_viewport_angle(config->cardboard_viewport_angle());
#endif
}

void GameOverState::AdvanceFrame(int delta_time, int* next_state) {
  world_->entity_manager.UpdateComponents(delta_time);
  UpdateMainCamera(&main_camera_, world_);

  // Return to the title screen after any key is hit.
  static const entity::WorldTime kMinTimeInEndState = 8000.0f;
  const bool event_over =
      world_->patron_component.event_time() > kMinTimeInEndState;
  const bool pointer_button_pressed =
      input_system_->GetPointerButton(0).went_down();
  const bool exit_button_pressed =
      input_system_->GetButton(FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(FPLK_AC_BACK).went_down();
  if (event_over && (pointer_button_pressed || exit_button_pressed)) {
    audio_engine_->PlaySound(sound_click_);

    // Stay in Cardboard unless the back button is pressed.
    *next_state = world_->is_in_cardboard() && pointer_button_pressed
                ? kGameStateGameplay : kGameStateGameMenu;
  }
}

void GameOverState::RenderPrep(Renderer* renderer) {
  world_->world_renderer->RenderPrep(main_camera_, *renderer, world_);
}

void GameOverState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_HMD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
}

void GameOverState::OnEnter(int /*previous_state*/) {
  world_->player_component.set_state(kPlayerState_NoProjectiles);
  UpdateMainCamera(&main_camera_, world_);

  // Stop the raft over the course of a few seconds.
  entity::EntityRef raft = world_->services_component.raft_entity();
  RailDenizenData* raft_rail_denizen =
      world_->rail_denizen_component.GetComponentData(raft);
  raft_rail_denizen->SetPlaybackRate(0.0f, kTimeToStopRaft);

  // Trigger the end-game event.
  static const entity::WorldTime kEndGameEventTime = 0;
  world_->patron_component.StartEvent(kEndGameEventTime);

#ifdef USING_GOOGLE_PLAY_GAMES
  if (gpg_manager_->LoggedIn()) {
    // Finished a game, post a score.
    auto player = world_->player_component.begin()->entity;
    auto attribute_data =
        world_->entity_manager.GetComponentData<AttributesData>(player);
    auto score = attribute_data->attributes[AttributeDef_PatronsFed];
    auto leaderboard_config = config_->gpg_config()->leaderboards();
    gpg_manager_->SubmitScore(
        leaderboard_config->LookupByKey(kGPGDefaultLeaderboard)->id()->c_str(),
        score);
  }
#endif
}

void GameOverState::OnExit(int next_state) {
  world_->patron_component.StopEvent();

  if (next_state == kGameStateGameplay) {
    LoadWorldDef(world_, config_->world_def());
  }
}

}  // zooshi
}  // fpl
