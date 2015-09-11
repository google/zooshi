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

// Update music gain based on lap number. This logic will eventually live in
// an event graph.
static void UpdateMusic(entity::EntityManager* entity_manager,
                        int* previous_lap, float* percent, int delta_time,
                        pindrop::Channel* music_channel_1,
                        pindrop::Channel* music_channel_2,
                        pindrop::Channel* music_channel_3) {
  entity::EntityRef raft =
      entity_manager->GetComponent<ServicesComponent>()->raft_entity();
  RailDenizenData* raft_rail_denizen =
      entity_manager->GetComponentData<RailDenizenData>(raft);
  int current_lap = static_cast<int>(raft_rail_denizen->lap);
  if (current_lap != *previous_lap) {
    pindrop::Channel* channels[] = {music_channel_1, music_channel_2,
                                    music_channel_3};
    const float kCrossFadeDuration = 5.0f;
    bool done = false;
    float seconds = delta_time / 1000.0f;
    float delta = seconds / kCrossFadeDuration;
    pindrop::Channel* channel_previous = channels[*previous_lap % 3];
    pindrop::Channel* channel_current = channels[current_lap % 3];
    *percent += delta;
    if (*percent >= 1.0f) {
      *percent = 1.0f;
      done = true;
    }
    // Equal power crossfade
    //    https://www.safaribooksonline.com/library/view/web-audio-api/9781449332679/s03_2.html
    // TODO: Add utility functions to Pindrop for this.
    float gain_previous = cos(*percent * 0.5f * static_cast<float>(M_PI));
    float gain_current = cos((1.0f - *percent) * 0.5f * static_cast<float>(M_PI));
    channel_previous->SetGain(gain_previous);
    channel_current->SetGain(gain_current);

    if (done) {
      *previous_lap = current_lap;
      *percent = 0.0f;
    }
  }
}

void GameplayState::AdvanceFrame(int delta_time, int* next_state) {
  // Update the world.
  world_->entity_manager.UpdateComponents(delta_time);
  UpdateMainCamera(&main_camera_, world_);
  UpdateMusic(&world_->entity_manager, &previous_lap_, &percent_, delta_time,
              &music_channel_lap_1_, &music_channel_lap_2_,
              &music_channel_lap_3_);

  if (input_system_->GetButton(fpl::FPLK_F9).went_down()) {
    world_->draw_debug_physics = !world_->draw_debug_physics;
  }
  if (input_system_->GetButton(fpl::FPLK_F8).went_down()) {
    world_->skip_rendermesh_rendering = !world_->skip_rendermesh_rendering;
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
                               const Config* config,
                               const InputConfig* input_config,
                               editor::WorldEditor* world_editor,
                               pindrop::AudioEngine* audio_engine) {
  input_system_ = input_system;
  world_ = world;
  input_config_ = input_config;
  world_editor_ = world_editor;
  audio_engine_ = audio_engine;

  sound_pause_ = audio_engine->GetSoundHandle("pause");
  music_gameplay_lap_1_ = audio_engine->GetSoundHandle("music_gameplay_lap_1");
  music_gameplay_lap_2_ = audio_engine->GetSoundHandle("music_gameplay_lap_2");
  music_gameplay_lap_3_ = audio_engine->GetSoundHandle("music_gameplay_lap_3");

#ifdef ANDROID_CARDBOARD
  cardboard_camera_.set_viewport_angle(config->cardboard_viewport_angle());
#else
  (void)config;
#endif
}

void GameplayState::OnEnter() {
  world_->player_component.set_active(true);
  input_system_->SetRelativeMouseMode(true);
  music_channel_lap_1_ =
      audio_engine_->PlaySound(music_gameplay_lap_1_, mathfu::kZeros3f, 1.0f);
  music_channel_lap_2_ =
      audio_engine_->PlaySound(music_gameplay_lap_2_, mathfu::kZeros3f, 0.0f);
  music_channel_lap_3_ =
      audio_engine_->PlaySound(music_gameplay_lap_3_, mathfu::kZeros3f, 0.0f);
  if (world_->is_in_cardboard) {
#ifdef ANDROID_CARDBOARD
    world_->services_component.set_camera(&cardboard_camera_);
#endif
  } else {
    world_->services_component.set_camera(&main_camera_);
  }
#ifdef ANDROID_CARDBOARD
  input_system_->cardboard_input().ResetHeadTracker();
#endif  // ANDROID_CARDBOARD
}

void GameplayState::OnExit() {
  music_channel_lap_1_.Stop();
  music_channel_lap_2_.Stop();
  music_channel_lap_3_.Stop();
}

}  // fpl_project
}  // fpl
