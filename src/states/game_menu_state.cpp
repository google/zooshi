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

#include "states/game_menu_state.h"

#include "components/sound.h"
#include "config_generated.h"
#include "events/play_sound.h"
#include "events_generated.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/input.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"
#include "states/states.h"
#include "states/states_common.h"
#include "world.h"

#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

void GameMenuState::Initialize(InputSystem* input_system, World* world,
                               BasePlayerController* input_controller,
                               const Config* config,
                               AssetManager* asset_manager,
                               FontManager* font_manager,
                               pindrop::AudioEngine* audio_engine) {
  world_ = world;

  // Set references used in GUI.
  input_system_ = input_system;
  asset_manager_ = asset_manager;
  font_manager_ = font_manager;
  audio_engine_ = audio_engine;

  sound_start_ = audio_engine->GetSoundHandle("start");
  sound_click_ = audio_engine->GetSoundHandle("click");

  // Set menu state.
  menu_state_ = kMenuStateStart;
  show_about_ = false;
  show_licences_ = false;
  show_how_to_play_ = false;
  show_audio_ = false;

  // Set the world def to load upon entering this state.
  world_def_ = config->world_def();
  input_controller_ = input_controller;
}

void GameMenuState::AdvanceFrame(int delta_time, int* next_state) {
  world_->entity_manager.UpdateComponents(delta_time);
  UpdateMainCamera(&main_camera_, world_);

  // Exit the game.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(FPLK_AC_BACK).went_down()) {
    *next_state = kGameStateExit;
  }

  if (menu_state_ == kMenuStateFinished || menu_state_ == kMenuStateCardboard) {
    *next_state = kGameStateGameplay;
    audio_engine_->PlaySound(sound_start_);
    world_->is_in_cardboard = (menu_state_ == kMenuStateCardboard);
  }
  menu_state_ = kMenuStateStart;
}

void GameMenuState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_CARDBOARD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);

  // No culling when drawing the menu.
  renderer->SetCulling(Renderer::kNoCulling);

  switch (menu_state_) {
    case kMenuStateStart:
      menu_state_ = StartMenu(*asset_manager_, *font_manager_, *input_system_);
      break;
    case kMenuStateOptions:
      menu_state_ = OptionMenu(*asset_manager_, *font_manager_, *input_system_);
      break;
    default:
      break;
  }
}

void GameMenuState::OnEnter() {
  LoadWorldDef(world_, world_def_, input_controller_);
  world_->player_component.set_active(false);
  input_system_->SetRelativeMouseMode(false);
}

}  // fpl_project
}  // fpl
