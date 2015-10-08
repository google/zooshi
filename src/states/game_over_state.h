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

#ifndef ZOOSHI_GAME_OVER_STATE_H_
#define ZOOSHI_GAME_OVER_STATE_H_

#include "camera.h"
#include "fplbase/input.h"
#include "pindrop/pindrop.h"
#include "states/gameplay_state.h"
#include "states/game_menu_state.h"
#include "states/state_machine.h"
#include "world.h"

namespace fpl {
namespace zooshi {

class GameOverState : public StateNode {
 public:
  void Initialize(InputSystem* input_system, World* world, const Config* config,
                  AssetManager* asset_manager, FontManager* font_manager,
                  GPGManager* gpg_manager_, pindrop::AudioEngine* audio_engine);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void RenderPrep(Renderer* renderer);
  virtual void Render(Renderer* renderer);
  virtual void HandleUI(Renderer* renderer);
  virtual void OnEnter(int previous_state);

 private:
  void GameOverScreen(AssetManager& assetman, FontManager& fontman,
                      InputSystem& input);

  // The world to display in the background.
  World* world_;

  // The config data to drive the loaderboard display.
  const Config* config_;

  // The camera(s) to use to render the background world.
  Camera main_camera_;
#ifdef ANDROID_CARDBOARD
  Camera cardboard_camera_;
#endif

  // FlatUI uses InputSystem for an input handling for a touch, gamepad,
  // mouse and keyboard.
  InputSystem* input_system_;

  // FlatUI loads resources using AssetManager.
  AssetManager* asset_manager_;

  // FlatUI uses FontManager for a text rendering.
  FontManager* font_manager_;

  // Used to display leaderboard.
  GPGManager* gpg_manager_;

  // The audio engine, so that sound effects can be played.
  pindrop::AudioEngine* audio_engine_;

  // Cache the common sounds that are going to be played.
  pindrop::SoundHandle sound_click_;

  // Textures used in menu UI.
  Texture* background_game_over_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_GAME_OVER_STATE_H_
