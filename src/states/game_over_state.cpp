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
namespace fpl_project {

void GameOverState::GameOverScreen(AssetManager& assetman, FontManager& fontman,
                                   InputSystem& input) {
  entity::EntityRef player = world_->services_component.player_entity();
  AttributesData* player_attributes =
      world_->attributes_component.GetComponentData(player);
  int score = player_attributes->attributes[AttributeDef_PatronsFed];

  gui::Run(assetman, fontman, input, [&]() {
    gui::StartGroup(gui::kLayoutHorizontalTop, 0);
    // Background image.
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    // Positioning the UI slightly above of the center.
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(0, -150));
    gui::Image(*background_game_over_, 850);
    gui::EndGroup();

    gui::SetTextColor(kColorBrown);

    // Menu items. Note that we are layering 2 layouts here
    // (background + menu items).
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(0, -150));
    gui::SetMargin(gui::Margin(200, 280, 200, 100));

    char buffer[32] = {0};
    sprintf(buffer, "Score: %i", score);

    gui::Label("Game Over", kMenuSize);
    gui::Label(static_cast<const char*>(buffer), kMenuSize);
    gui::EndGroup();
    gui::EndGroup();
  });
}

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

#ifdef ANDROID_CARDBOARD
  cardboard_camera_.set_viewport_angle(config->cardboard_viewport_angle());
#endif
}

void GameOverState::AdvanceFrame(int delta_time, int* next_state) {
  world_->entity_manager.UpdateComponents(delta_time);
  UpdateMainCamera(&main_camera_, world_);

  // Stop the raft over the course of a few seconds.
  entity::EntityRef raft = world_->services_component.raft_entity();
  RailDenizenData* raft_rail_denizen =
      world_->rail_denizen_component.GetComponentData(raft);
  raft_rail_denizen->spline_playback_rate -= delta_time / 1000.f;
  if (raft_rail_denizen->spline_playback_rate < 0.0f) {
    raft_rail_denizen->spline_playback_rate = 0.0f;
  }
  raft_rail_denizen->motivator.SetSplinePlaybackRate(
      raft_rail_denizen->spline_playback_rate);

  // Return to the title screen after any key is hit.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(FPLK_AC_BACK).went_down() ||
      input_system_->GetPointerButton(0).went_down()) {
    audio_engine_->PlaySound(sound_click_);
    *next_state = kGameStateGameMenu;
  }
}

void GameOverState::RenderPrep(Renderer* renderer) {
  world_->world_renderer->RenderPrep(main_camera_, *renderer, world_);
}

void GameOverState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_CARDBOARD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
}

void GameOverState::HandleUI(Renderer* renderer) {
  // No culling when drawing the game over screen.
  renderer->SetCulling(Renderer::kNoCulling);
  GameOverScreen(*asset_manager_, *font_manager_, *input_system_);
}

void GameOverState::OnEnter(int /*previous_state*/) {
  world_->player_component.set_active(false);
  world_->is_in_cardboard = false;
  input_system_->SetRelativeMouseMode(false);
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

    // Fill in Leaderboard list.
    gpg_manager_->ShowLeaderboards(
        leaderboard_config->LookupByKey(kGPGDefaultLeaderboard)->id()->c_str());
  }
#endif
}

}  // fpl_project
}  // fpl
