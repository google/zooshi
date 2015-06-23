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

#include "game_menu_state.h"

#include "components/sound.h"
#include "components/transform.h"
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
#include "states.h"
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

void GameMenuState::Initialize(Renderer* renderer, InputSystem* input_system,
                               World* world, const InputConfig* input_config,
                               editor::WorldEditor* world_editor,
                               AssetManager* asset_manager,
                               FontManager* font_manager) {
  GameplayState::Initialize(renderer, input_system, world, input_config,
                            world_editor);

  // Set references used in GUI.
  input_system_ = input_system;
  asset_manager_ = asset_manager;
  font_manager_ = font_manager;

  // Show cursor temporarily.
  input_system_->SetRelativeMouseMode(false);

  // Set menu state.
  menu_state_ = kMenuStateStart;
  show_about_ = false;
  show_licences_ = false;
  show_how_to_play_ = false;
  show_audio_ = false;
}

void GameMenuState::AdvanceFrame(int delta_time, int* next_state) {
  GameplayState::AdvanceFrame(delta_time, next_state);

  // Exit the game.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down()) {
    *next_state = kGameStateExit;
  }

  if (menu_state_ == kMenuStateFinished) {
    *next_state = kGameStateGameplay;
    // Hide cursor.
    input_system_->SetRelativeMouseMode(true);
  }
}

void GameMenuState::Render(Renderer* renderer) {
  GameplayState::Render(renderer);

  switch (menu_state_) {
    case kMenuStateStart:
      menu_state_ =
          StartMenu(*asset_manager_, *font_manager_, *input_system_);
      break;
    case kMenuStateOptions:
      menu_state_ =
          OptionMenu(*asset_manager_, *font_manager_, *input_system_);
      break;
    default:
      break;
  }
}

}  // fpl_project
}  // fpl
