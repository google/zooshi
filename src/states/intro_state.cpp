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

#include <limits.h>

#include "states/intro_state.h"

#include "fplbase/input.h"
#include "full_screen_fader.h"
#include "states/states_common.h"
#include "world.h"

using fpl::component_library::MetaComponent;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;

namespace fpl {
namespace zooshi {

static const int kFadeTimerPending = INT_MAX;
static const int kFadeTimerComplete = INT_MAX - 1;
static const int kFadeWaitTime = 500;  // In milliseconds.
static const char* kMasterBus = "master";

IntroState::IntroState()
    : world_(nullptr),
      input_system_(nullptr),
      fader_(nullptr),
      fade_timer_(kFadeTimerPending) {}

void IntroState::Initialize(InputSystem* input_system, World* world,
                            const Config* config, FullScreenFader* fader,
                            pindrop::AudioEngine* audio_engine) {
  input_system_ = input_system;
  world_ = world;
  fader_ = fader;
  master_bus_ = audio_engine->FindBus(kMasterBus);

#ifdef ANDROID_CARDBOARD
  cardboard_camera_.set_viewport_angle(config->cardboard_viewport_angle());
#else
  (void)config;
#endif
}

void IntroState::HideBox(bool hide) {
  // TODO: find a better way to get the entity than by string name.
  auto entity = world_->meta_component.GetEntityFromDictionary("introbox-1");
  world_->render_mesh_component.SetHiddenRecursively(entity, hide);
}

void IntroState::AdvanceFrame(int delta_time, int* next_state) {
  // Update components so that the player can throw sushi.
  world_->entity_manager.UpdateComponents(delta_time);
  // Update camera so that the player can look around.
  UpdateMainCamera(&main_camera_, world_);

  auto player = world_->player_component.begin()->entity;
  auto player_data =
      world_->entity_manager.GetComponentData<PlayerData>(player);

  // Start countdown timer before we fade to game.
  if (player_data->input_controller()->Button(kFireProjectile).Value() &&
      player_data->input_controller()->Button(kFireProjectile).HasChanged() &&
      fade_timer_ == kFadeTimerPending) {
    fade_timer_ = kFadeWaitTime;
  }

  // Fade to game.
  if (fade_timer_ <= 0) {
    fader_->Start(kIntroStateFadeTransitionDuration, mathfu::kZeros3f,
                  kFadeOutThenIn, vec3(-1.0f, 1.0f, 0.0f),
                  vec3(1.0f, -1.0f, 0.0f));
    fade_timer_ = kFadeTimerComplete;
  }

  // Update the fade timer, if it's active.
  if (fade_timer_ != kFadeTimerPending && fade_timer_ != kFadeTimerComplete) {
    fade_timer_ -= delta_time;
  }

  // Go back to menu.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(FPLK_AC_BACK).went_down()) {
    *next_state = kGameStateGameMenu;
  }

  if (fader_->AdvanceFrame(delta_time)) {
    HideBox(true);
    // Enter the game.
    *next_state = kGameStateGameplay;
  }
}

void IntroState::RenderPrep(Renderer* renderer) {
  world_->world_renderer->RenderPrep(main_camera_, *renderer, world_);
}

void IntroState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_CARDBOARD
  cardboard_camera = &cardboard_camera_;
#endif  // ANDROID_CARDBOARD
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
  if (!fader_->Finished()) {
    renderer->set_model_view_projection(
          mat4::Ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
    fader_->Render(renderer);
  }
}

void IntroState::OnEnter(int /*previous_state*/) {
  world_->player_component.set_active(true);
  input_system_->SetRelativeMouseMode(true);
#ifdef ANDROID_CARDBOARD
  input_system_->cardboard_input().ResetHeadTracker();
#endif  // ANDROID_CARDBOARD
  // Move the player inside the intro box.
  auto player = world_->player_component.begin()->entity;
  auto player_transform =
      world_->entity_manager.GetComponentData<TransformData>(player);
  // TODO(proppy): get position of the introbox entity
  player_transform->position += mathfu::vec3(0, 0, 500);
  fade_timer_ = kFadeTimerPending;
  HideBox(false);
  master_bus_.FadeTo(0.0f, kFadeWaitTime / kMillisecondsPerSecond);
}

void IntroState::OnExit(int /*next_state*/) {
  // Move the player back on the river trail.
  auto player = world_->player_component.begin()->entity;
  auto player_transform =
      world_->entity_manager.GetComponentData<TransformData>(player);
  player_transform->position = mathfu::vec3(0, 0, 0);
  master_bus_.FadeTo(1.0f, kFadeWaitTime / kMillisecondsPerSecond);
}

}  // zooshi
}  // fpl
