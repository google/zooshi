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

#include "states/intro_state.h"

#include "fplbase/input.h"
#include "states/states_common.h"
#include "world.h"

using fpl::component_library::MetaComponent;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;

namespace fpl {
namespace fpl_project {

void IntroState::Initialize(InputSystem* input_system, World* world) {
  input_system_ = input_system;
  world_ = world;
}

void IntroState::AdvanceFrame(int delta_time, int* next_state) {
  // Update components so that the player can throw sushi.
  world_->entity_manager.UpdateComponents(delta_time);
  // Update camera so that the player can look around.
  UpdateMainCamera(&main_camera_, world_);

  auto player = world_->player_component.begin()->entity;
  auto player_data =
      world_->entity_manager.GetComponentData<PlayerData>(player);

  // Enter game.
  if (player_data->input_controller()->Button(kFireProjectile).Value() &&
      player_data->input_controller()->Button(kFireProjectile).HasChanged()) {
    // TODO(proppy): add a pause
    // TODO(proppy): fade to black
    auto entity = world_->meta_component.GetEntityFromDictionary("introbox-1");
    world_->render_mesh_component.SetHiddenRecursively(entity, true);
    *next_state = kGameStateGameplay;
  }

  // Go back to menu.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(FPLK_AC_BACK).went_down()) {
    *next_state = kGameStateGameMenu;
  }
}

void IntroState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_CARDBOARD
  cardboard_camera = &cardboard_camera_;
#endif // ANDROID_CARDBOARD
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
}

void IntroState::OnEnter(int /*previous_state*/) {
  world_->player_component.set_active(true);
  input_system_->SetRelativeMouseMode(true);
#ifdef ANDROID_CARDBOARD
  input_system_->cardboard_input().ResetHeadTracker();
#endif // ANDROID_CARDBOARD
  // Move the player inside the intro box.
  auto player = world_->player_component.begin()->entity;
  auto player_transform =
      world_->entity_manager.GetComponentData<TransformData>(player);
  // TODO(proppy): get position of the introbox entity
  player_transform->position += mathfu::vec3(0, 0, -30);
}

void IntroState::OnExit(int /*next_state*/) {
  // Move the player back on the river trail.
  auto player = world_->player_component.begin()->entity;
  auto player_transform =
      world_->entity_manager.GetComponentData<TransformData>(player);
  player_transform->position = mathfu::vec3(0, 0, 0);
}

} // fpl_project
} // fpl
