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

#include "states/gameplay_state.h"

#include "fplbase/input.h"
#include "input_config_generated.h"
#include "mathfu/glsl_mappings.h"
#include "states/states.h"
#include "states/states_common.h"
#include "world.h"

using mathfu::vec2i;
using mathfu::vec2;

namespace fpl {
namespace fpl_project {

static vec2 AdjustedMouseDelta(const vec2i& raw_delta,
                               const InputConfig& input_config) {
  vec2 delta(raw_delta);
  delta *= input_config.mouse_sensitivity();
  if (!input_config.invert_x()) {
    delta.x() *= -1.0f;
  }
  if (!input_config.invert_y()) {
    delta.y() *= -1.0f;
  }
  return delta;
}

void GameplayState::AdvanceFrame(int delta_time, int* next_state) {
  // Update the world.
  world_->entity_manager.UpdateComponents(delta_time);
  UpdateMainCamera(&main_camera_, world_);

  if (input_system_->GetButton(fpl::FPLK_F9).went_down()) {
    world_->draw_debug_physics = !world_->draw_debug_physics;
  }

  // Switch States if necessary.
  if (world_editor_ && input_system_->GetButton(fpl::FPLK_F10).went_down()) {
    world_editor_->SetInitialCamera(main_camera_);
    *next_state = kGameStateWorldEditor;
  }

  // Pause the game.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(FPLK_AC_BACK).went_down()) {
    audio_engine_->PlaySound(sound_pause_);
    *next_state = kGameStatePause;
  }
}

void GameplayState::Render(Renderer* renderer) {
  if (!world_->asset_manager) return;
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_CARDBOARD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
}

void GameplayState::Initialize(InputSystem* input_system, World* world,
                               const InputConfig* input_config,
                               editor::WorldEditor* world_editor,
                               pindrop::AudioEngine* audio_engine) {
  input_system_ = input_system;
  world_ = world;
  input_config_ = input_config;
  world_editor_ = world_editor;
  audio_engine_ = audio_engine;

  sound_pause_ = audio_engine->GetSoundHandle("pause");
}

void GameplayState::OnEnter() {
  world_->player_component.set_active(true);
  input_system_->SetRelativeMouseMode(true);
#ifdef ANDROID_CARDBOARD
  input_system_->cardboard_input().ResetHeadTracker();
#endif  // ANDROID_CARDBOARD
}

}  // fpl_project
}  // fpl
