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

#ifndef ZOOSHI_GAME_MENU_STATE_H_
#define ZOOSHI_GAME_MENU_STATE_H_

#include "camera.h"
#include "flatui/flatui.h"
#include "flatui/flatui_common.h"
#include "fplbase/input.h"
#include "pindrop/pindrop.h"
#include "states/gameplay_state.h"
#include "states/state_machine.h"

namespace fpl {

class InputSystem;

namespace fpl_project {

class BasePlayerController;
struct Config;
struct InputConfig;
struct WorldDef;

enum MenuState {
  kMenuStateStart,
  kMenuStateOptions,
  kMenuStateFinished,
  kMenuStateCardboard,
};

// Constant defintions for UI elements. Colors, button sizes etc.
const auto kColorBrown = vec4(0.37f, 0.24f, 0.09f, 1.0f);
const auto kColorLightBrown = vec4(0.82f, 0.77f, 0.60f, 1.0f);
const auto kPressedColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
const auto kHoverColor = vec4::Min(kColorBrown * 1.5f, mathfu::kOnes4f);
const auto kMenuSize = 80;
const auto kButtonSize = 55;

class GameMenuState : public StateNode {
 public:
  void Initialize(InputSystem* input_system, World* world,
                  BasePlayerController* player_controller, const Config* config,
                  AssetManager* asset_manager, FontManager* font_manager,
                  pindrop::AudioEngine* audio_engine);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(Renderer* renderer);
  virtual void OnEnter();
  virtual void OnExit();

 private:
  MenuState StartMenu(AssetManager& assetman, FontManager& fontman,
                      InputSystem& input);
  MenuState OptionMenu(AssetManager& assetman, FontManager& fontman,
                       InputSystem& input);
  // The world to display in the background.
  World* world_;

  // The camera(s) to use to render the background world.
  Camera main_camera_;
#ifdef ANDROID_CARDBOARD
  Camera cardboard_camera_;
#endif

  // IMGUI uses InputSystem for an input handling for a touch, gamepad,
  // mouse and keyboard.
  InputSystem* input_system_;

  // IMGUI loads resources using AssetManager.
  AssetManager* asset_manager_;

  // IMGUI uses FontManager for a text rendering.
  FontManager* font_manager_;

  // The audio engine, so that sound effects can be played.
  pindrop::AudioEngine* audio_engine_;

  // Cache the common sounds that are going to be played.
  pindrop::SoundHandle sound_start_;
  pindrop::SoundHandle sound_click_;

  // This will eventually be removed when there are events to handle this logic.
  pindrop::SoundHandle music_menu_;
  pindrop::Channel music_channel_;

  // Menu state.
  MenuState menu_state_;

  // The world definition to load upon entering the menu.
  const WorldDef* world_def_;

  // The controller to use for the main character when the world is created.
  BasePlayerController* input_controller_;

  // Textures used in menu UI.
  Texture* background_title_;
  Texture* background_options_;
  Texture* button_back_;

  // In-game menu state.
  bool show_about_;
  bool show_licences_;
  bool show_how_to_play_;
  bool show_audio_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAME_MENU_STATE_H_
